#include "types.h"
#include "user.h"

#define MAX_ROUNDS 10
#define TIME 1000

int main(int argc, char* argv[])
{
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