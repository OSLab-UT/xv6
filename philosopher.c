#include "unistd.h"
#include "user.h"

#define MAX_ROUNDS 10
#define TIME 1

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
<<<<<<< HEAD
    counter = 0;
    while(counter < MAX_ROUNDS){
=======
    while(1){
>>>>>>> 843daaea2b5972c0a29d3f72b56c2d02b000309d
        state = THINKING;
        printf("%s is thinking.\n", name);
        sleep(TIME);
        state = HUNGRY;
        printf("%s is hungry.\n", name);
        // get semaphores  
        sem_acquire(first);
        print("%s get fork %d.\n", name, first);
        sem_acquire(last);
        print("%s get fork %d.\n", name, last);
        state = EATTING;
        print("%s start eatting.\n", name);
        sleep(TIME);
        print("%s finish eatting.\n", name);
        state = FINISHED;
        sem_release(first);
        sem_release(last);
        counter++;
    }
    exit();
}