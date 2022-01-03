// semaphore lock

struct semaphore {
    uint max;           // max number of processes that can get semaphore simultaneously
    uint locked;        // Is the lock held?
    //uint run[NPIS];     // Queue of processes running in semaphore
    //uint run_first;     // first of running queue
    //uint run_last;      // end of running queue
    uint run_num;       // number of processes in running queue
    int wait[NPIS];    // Queue of processes waiting for semaphore to be released
    uint wait_first;    // first of waiting queue
    uint wait_last;     // end of waiting queue
    uint wait_num;      // number of processes in waiting queue

    // For debugging:
    char *name;         // Name of semaphore.
};