#include "semaphore.h"
#include "proc.h"

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

