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
  if(argint(0, &pid) < 0 || argint(0, &queueIndex) < 0)
    return -1;
  
  struct proc* process = findprocbypid(pid);
  if(process == 0)
    return -1;
  if(process->queueIndex >= NQUEUE || process->queueIndex < 0)
    return -1;
  int procQueueFront = schedulingQueues[process->queueIndex].front;
  if(schedulingQueues[process->queueIndex].array[procQueueFront] != process)
    return -1;
  
  LIFO_dequeue(&schedulingQueues[process->queueIndex]);
  enqueue(&schedulingQueues[queueIndex], process);
  return 0;
}

int
sys_printallprocesses(void)
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
}