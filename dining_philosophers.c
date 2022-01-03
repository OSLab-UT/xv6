#include "types.h"
#include "user.h"

char* names[5] = {"first ph", "second ph", "third ph", "forth ph", "fifth ph"};

int getNumOfDigits(int x)
{
    if(x < 10)
        return 1;
    int digits = 0;
    while(x > 0)
    {
        digits++;
        x /= 10;
    }
    return digits;
}

char* intToString(int x)
{
    int wordSize = getNumOfDigits(x);
    wordSize += 2;
    char* str = (char*) malloc(wordSize);
    str[wordSize - 1] = '\0';
    str[wordSize - 2] = ' ';

    for(int i = wordSize - 3; i >= 0; i--)
    {
        str[i] = (x % 10) + '0';
        x /= 10;
    }
    return str;
}

char* argv[] = {"philosopher", "0", "first"};

int main(){
    for(int i = 0; i < 5; i++){
        sem_init(i, 1);
    }
    for(int i = 0; i < 5; i++){
        int pid = fork();
        if(pid < 0){
            printf(2, "fork failed.\n");
            exit();
        }
        else if(pid > 0){
            argv[1] = intToString(i);
            argv[2] = names[i];
            exec("philosopher", argv);
            printf(2, "exec philosopher failed.\n");
            exit();
        }
    }
    exit();
}