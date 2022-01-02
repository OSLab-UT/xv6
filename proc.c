#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
//////////////////////
// #include "stdio.h"
////////////////////

//struct Queue LCFS_queue;
//struct Queue RR_scheduler;

struct Queue {
  int front, rear, size;
  struct spinlock lock;
  struct proc* array[NPROC];
};
struct Queue schedulingQueues[NQUEUE];

struct  {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;
extern struct Queue schedulingQueues[NQUEUE];

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

int isEmpty(struct Queue* queue);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->ctime = ticks; // Initialize creation time for process
  p->rtime = 0; 
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
  p->isBeingDebugged = 0;
  p->debugger = 0;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;
  curproc->etime = ticks;
  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
    if(p->isBeingDebugged && p->debugger == curproc)
    {
      p->debugger = 0;
      p->isBeingDebugged = 0;
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;

      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->debugger = 0;
        p->isBeingDebugged = 0;
        p->state = UNUSED;
        p->ctime = 0; // Reinitialising creation time of process
        p->etime = 0; // Reinitialising end time of process
        p->rtime = 0; // Reinitialising run time of process
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.

//////////////
void run_first_proc(struct cpu *c){
  struct proc *p;
  
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE)
      continue;

    if(p->pid < 3){
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&(c->scheduler), p->context);
      switchkvm();
      c->proc = 0;
    }
  }    
  release(&ptable.lock);
}
/////////////

void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);



    /* In this place we should replce the below for loop
      with a loop on queues and in this loop we should write
      a loop on each queue */


    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }

  /*for(;;){
    // Enable interrupts on this processor.
    sti();
    add_new_process_to_queues();
    ageing();
    if(isEmpty(&schedulingQueues[RR_QUEUE_INDEX]) == 0){
      RR_scheduler(c);
    }
    else if(isEmpty(&schedulingQueues[LCFS_QUEUE_INDEX]) == 0){
      LCFS_scheduler(c);
    }
    else if(isEmpty(&schedulingQueues[MHRRN_QUEUE_INDEX]) == 0){
      MHRRN_scheduler(c);
    }
  }*/
}

void add_new_process_to_queues(void)
{
  struct proc* p;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE)
      continue;

    ///////////
    if(p->pid < 3)
      continue;
    /////

    int found = 0;
    for(int i = 0; i < NQUEUE; i++){
      struct Queue* queue = &schedulingQueues[i];
      for (int i = 0; i < queue->size; i++){
        int curr = (queue->front + i) % NPROC;
        if (queue->array[curr]->pid == p->pid){
          found = 1;
          break;
        } 
      }
      if(found)
        break;
    }
    if(found)
      continue;

    // first initialization
    p->ExeCycleNum = 0;
    p->HRRNpriority = 1;
    p->queueIndex = LCFS_QUEUE_INDEX;
    p->age = 0;
    //////////////////////
    cprintf("Add process : %d\n", p->pid);
    //panic("add process");
    /////////////////   
    enqueue(LCFS_QUEUE_INDEX, p);
  }
  release(&ptable.lock);
}

void ageing(void)
{
  struct Queue* queue = &schedulingQueues[LCFS_QUEUE_INDEX];
  for (int i = 0; i < queue->size; i++){
    int curr = (queue->front + i) % NPROC;
    if (queue->array[curr]->age >= AGE_LIMIT){
      struct proc* old = index_dequeue(queue, curr);
      old->age = 0;
      old->queueIndex = RR_QUEUE_INDEX;
      enqueue(RR_QUEUE_INDEX, old);
      i--;
      continue;
    } 
    else{
      queue->array[curr]->age += 1;
    }
  }
  queue = &schedulingQueues[MHRRN_QUEUE_INDEX];
  for (int i = 0; i < queue->size; i++){
    int curr = (queue->front + i) % NPROC;
    if (queue->array[curr]->age >= AGE_LIMIT){
      struct proc* old = index_dequeue(queue, curr);
      old->age = 0;
      enqueue(RR_QUEUE_INDEX, old);
      i--;
      continue;
    } 
    else{
      queue->array[curr]->age += 1;
    }
  }
}

void MHRRN_scheduler(struct cpu *c){
  struct proc *p;
  
  acquire(&schedulingQueues[MHRRN_QUEUE_INDEX].lock);
    
  p = MHRRN_dequeue(&schedulingQueues[MHRRN_QUEUE_INDEX]);
  while(p->state == RUNNABLE){
    c->proc = p;
    switchuvm(p);
    p->state = RUNNING;
    swtch(&(c->scheduler), p->context);
    switchkvm();
    c->proc = 0;
  }
  release(&schedulingQueues[MHRRN_QUEUE_INDEX].lock);
}

void LCFS_scheduler(struct cpu *c){
  struct proc *p;
  
  acquire(&schedulingQueues[LCFS_QUEUE_INDEX].lock);
    
  p = LIFO_dequeue(LCFS_QUEUE_INDEX);
  while(p->state == RUNNABLE){
    c->proc = p;
    switchuvm(p);
    p->state = RUNNING;
    swtch(&(c->scheduler), p->context);
    switchkvm();
    c->proc = 0;
  }
  release(&schedulingQueues[LCFS_QUEUE_INDEX].lock);
}

void RR_scheduler(struct cpu *c){
  struct proc *p;

  acquire(&schedulingQueues[RR_QUEUE_INDEX].lock);

  p = LIFO_dequeue(RR_QUEUE_INDEX);
  switchuvm(p);
  p->state = RUNNING;
  swtch(&(c->scheduler), p->context);
  switchkvm();
  if(p->etime==0)
    enqueue(RR_QUEUE_INDEX, p);
  // Process is done running for now.
  // It should have changed its p->state before coming back.
  c->proc = 0;
  release(&schedulingQueues[RR_QUEUE_INDEX].lock);
}

// create queue with default capacity.
struct Queue create_queue(){
  struct Queue queue;
  queue.front=0;
  queue.rear=0;
  queue.size=0;
  return queue;

}
int isEmpty(struct Queue* queue)
{
  return (queue->size == 0);
}

int isFull(struct Queue* queue)
{
  return (queue->size == NPROC);
}

void enqueue(int queueIndex, struct proc* item)
{
  if (isFull(&schedulingQueues[queueIndex]))
    panic("Queue is full.");
  schedulingQueues[queueIndex].rear = (schedulingQueues[queueIndex].rear + 1) % NPROC;
  schedulingQueues[queueIndex].array[schedulingQueues[queueIndex].rear] = item;
  schedulingQueues[queueIndex].size = schedulingQueues[queueIndex].size + 1;
}

// dequeue for RR and LCFS
struct proc* LIFO_dequeue(int queueIndex)
{
  if (isEmpty(&schedulingQueues[queueIndex]))
    panic("Queue is empty.");
  struct proc* item = schedulingQueues[queueIndex].array[schedulingQueues[queueIndex].front];
  schedulingQueues[queueIndex].front = (schedulingQueues[queueIndex].front + 1) % NPROC;
  schedulingQueues[queueIndex].size = schedulingQueues[queueIndex].size + 1;
  return item;
}

// dequeue for MHRRN
struct proc* MHRRN_dequeue(struct Queue* queue)
{
  if (isEmpty(queue))
    panic("Queue is empty.");

  float max = -1000.0;
  int max_place = -1;
  for (int i = 0; i < queue->size; i++){
    int curr = (queue->front + i) % NPROC;
    int waiting_time = sys_uptime() - queue->array[curr]->ctime;
    // current time gave from int sys_uptime(void) function in line 82 sysproc.c
    int ECN = queue->array[curr]->ExeCycleNum;
    float HRRN = (waiting_time - ECN) / ECN;
    float MHRRN = (HRRN + queue->array[curr]->HRRNpriority) / 2;
    if (MHRRN > max){
      max = MHRRN;
      max_place = curr;
    }
  }
  struct proc* item = queue->array[queue->front];
  if (max_place != queue->front){
    struct proc* temp = queue->array[max_place];
    queue->array[max_place] = item;
    item = temp;
  }
  queue->front = (queue->front + 1) % NPROC;
  queue->size = queue->size - 1;
  return item;
}

// dequeue with index for ageing and change proc queue with syscall
struct proc* index_dequeue(struct Queue* queue, int index)
{
  if (isEmpty(queue))
    panic("Queue is empty.");
  struct proc* ret = queue->array[queue->front];
  if (index != queue->front){
    ret = queue->array[index];
  }

  struct proc* tempForPrevProc = 0;
  struct proc* secondTemp = 0;
  for(int i = queue->front; i <= index; i = (i+1)  % NPROC)
  {
    if(tempForPrevProc == 0)
    {
      tempForPrevProc = queue->array[i];
    }
    else
    {
      secondTemp = queue->array[i];
      queue->array[i] = tempForPrevProc;
      tempForPrevProc = secondTemp;
    }
  }

  queue->front = (queue->front + 1) % NPROC;
  queue->size = queue->size - 1;
  return ret;
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

struct proc*
findprocbypid(int pid)
{
  struct proc* proc = 0;
  acquire(&ptable.lock);

  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->pid == pid)
    {
      proc = p;
      break;
    }
  }
  release(&ptable.lock);
  return proc;
} 

int
getSchedulingQueueFront(int queueIndex)
{
  return schedulingQueues[queueIndex].front;
}

const char PROCESS_STATE[6][16] = {"UNUSED", "EMBRYO", "SLEEPING", "RUNNABLE", "RUNNING", "ZOMBIE"};

void printAllProcesses()
{
  acquire(&ptable.lock);
  struct proc* p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->state != UNUSED)
    {
      cprintf("%s %d %s %d %d %d\n", p->name, p->pid, PROCESS_STATE[p->state], p->queueIndex, p->ExeCycleNum, p->ctime);
    }
  }
  release(&ptable.lock);
}

int getProcessIndexInQueue(int queueIndex, struct proc* process)
{
  for(int i = schedulingQueues[queueIndex].front; i <= schedulingQueues[queueIndex].rear; i = (i+1) % NPROC)
  {
    if(schedulingQueues[queueIndex].array[i] == process)
      return i;
  }
  return -1;
}

struct proc* index_dequeue_help(int queueIndex, int procIndex)
{
  return index_dequeue(&schedulingQueues[queueIndex], procIndex);
}