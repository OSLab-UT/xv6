#include "unistd.h"
#include "user.h"

enum pstate { THINKING, EATTING, HUNGRY, FINISHED};






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
    enum pstate state;
    while(true){
        state = THINKING;
        printf("%s is thinking.\n", name);
        sleep(1);
        state = HUNGRY;
        printf("%s is hungry.\n", name);
        // get semaphores  
        sem_acquire(first);
        print("%s get fork %d.\n", name, first);
        sem_acquire(last);
        print("%s get fork %d.\n", name, last);
        state = EATTING;
        print("%s start eatting.\n", name);
        sleep(1);
        print("%s finish eatting.\n", name);
        state = FINISHED;
        sem_release(first);
        sem_release(last);
    }
}