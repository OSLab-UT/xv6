#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_digitsum(void)
{
  struct proc* currproc = myproc();

  int num = currproc->tf->eax;

  int sum = 0;
  while(num)
  {
    sum += num % 10;
    num /= 10;
  }
  return sum;
}

int
sys_setprocessparent(void)
{
  struct proc* curproc = myproc();
  int pid;
  if(argint(0, &pid) < 0)
    return -1;
  
  struct proc* childproc = findprocbypid(pid);
  if(childproc == 0)
    return -1;
  if(childproc->isBeingDebugged)
    return -1; 

  childproc->debugger = curproc;
  childproc->isBeingDebugged = 1;
  return 0;
}

int
sys_getparentpid(void)
{
  struct proc* parent = myproc()->parent;
  if(parent == 0)
    return parent->pid;
  return 0;
}

int
sys_changeprocessqueue(void)
{
  int pid, queueIndex;
  if(argint(0, &pid) < 0 || argint(1, &queueIndex) < 0)
    return -1;
  
  struct proc* process = findprocbypid(pid);
  if(process == 0)
    return -1;
  if(process->queueIndex >= NQUEUE || process->queueIndex < 0)
    return -1;
  process->age = 0;
  int index = getProcessIndexInQueue(process->queueIndex, process);
  index_dequeue_help(process->queueIndex, index);
  enqueue(queueIndex, process);
  return 0;
}

int
sys_printallprocesses(void)
{
  printAllProcesses();
  return 0;
}

int
sys_setMHRRNprocessspace(void)
{
  int MHRRNfactor;
  if(argint(0, &MHRRNfactor) < 0)
    return -1;
  struct proc* p = myproc();
  p->HRRNpriority = MHRRNfactor;
  return 0;
}

int
sys_setMHRRNkernelspace(void)
{
  int pid, MHRRNfactor;
  if(argint(0, &pid) < 0 || argint(1, &MHRRNfactor) < 0)
    return -1;
  
  struct proc* p = findprocbypid(pid);
  if(p == 0)
    return -1;
  p->HRRNpriority = MHRRNfactor;
  return 0;
}

int
sys_sem_init(void)
{
  int i, v;
  if(argint(0, &i) < 0 || argint(1, &v) < 0)
    return -1;
  
  sem_init_kernel(i, v);
  return 0;
}

int
sys_sem_acquire(void)
{
  int i;
  if(argint(0, &i) < 0)
    return -1;
  
  int flag = sem_acquire_kernel(i, myproc()->pid);

  if(flag == 0){
    myproc()->state = SLEEPING;
  }
  return 0;
}

int
sys_sem_release(void)
{
  int i;
  if(argint(0, &i) < 0)
    return -1;
  
  int pid = sem_release_kernel(i);

  if(pid != -1){
    struct proc* process = findprocbypid(pid);
    if(process == 0)
      return -1;
    if(process->queueIndex >= NQUEUE || process->queueIndex < 0)
      return -1;
    process->state = RUNNABLE;
  }
  return 0;
}

int
sys_get_free_pages_count(void)
{
  return get_free_pages_count();
}