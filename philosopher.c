#include "types.h"
#include "user.h"

#define MAX_ROUNDS 100
#define TIME 100

int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        printf(2, "Usage: number and name of philosopher.\n");
        exit();
    }
    printf(1, "%s.\n",argv[0]);
    int num = atoi(argv[1]);
    int first, last;
    if(num % 2 == 0){
        first = num;
        last = (num + 1) % 5;
    }
    else{
        last = num;
        first = (num + 1) % 5;
    }
    char* name = argv[2];
    int counter = 0;
    while(counter < MAX_ROUNDS){
        //state = THINKING;
        printf(1, "%s is thinking.\n", name);
        sleep(TIME);
        //state = HUNGRY;
        printf(1, "%s is hungry.\n", name);
        // get semaphores  
        sem_acquire(first);
        printf(1, "%s get fork %d.\n", name, first);
        sem_acquire(last);
        printf(1, "%s get fork %d.\n", name, last);
        //state = EATTING;
        printf(1, "%s start eatting.\n", name);
        sleep(TIME);
        printf(1, "%s finish eatting.\n", name);
        //state = FINISHED;
        sem_release(first);
        sem_release(last);
        counter++;
    }
    exit();
}