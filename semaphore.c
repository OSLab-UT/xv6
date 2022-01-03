#include "semaphore.h"
#include "proc.h"
#include "param.h"

void
sem_init(int i, int v)
{
    sems[i].max = v;
    sems[i].locked = 0;
    sems[i].run_first = 0;
    sems[i].run_last = 0;
    sems[i].run_num = 0;
    sems[i].wait_first = 0;
    sems[i].wait_last = 0;
    sems[i].wait_num = 0;
    // add i to name with sprintf
    // sems[i].name = "semaophre number i"
}

uint
sem_acquire(int i)
{
    if(sems[i].locked){
        return 0;
    }
    sems[i].run_num += 1;
    if(sems[i].run_num >= sems[i].max){
        sems[i].locked = 1;
    }
    return 1;
}

int
sem_release(int i)
{
    if(sems[i].wait_num){
        sems[i].wait_num -= 1;
        int proc = sems[i].wait[sems[i].wait_first];
        sems[i].wait_first = (sems[i].wait_first + 1) % NPIS;
        return proc;
    }
    return -1;
}

