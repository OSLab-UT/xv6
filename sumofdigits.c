#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"


void sumOfDigits(char* arg)
{
    int num = atoi(arg);
    int b;
    asm volatile("movl %1, %0;"
        :"=a"(b)        /* output */
        :"m"(num)         /* input */
        :        /* clobbered register */
        );      
    printf(1, "b %d\n", b);

    int sum = digitsum();
    printf(1, "The sum of the digits of %d is equal to %d\n", num, sum);
    return;
}

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf(2, "Usage: sum of digits number\n");
        exit();
    }
    else
    {
        sumOfDigits(argv[1]);
        exit();
    }
}