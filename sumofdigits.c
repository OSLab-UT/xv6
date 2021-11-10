#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"


void sumOfDigits(char* arg)
{
    int num = atoi(arg);
    asm ("movl %1, %%eax;"
        :"r"(num) /* input */
        :"%eax" /* clobbered register */
        );

    int sum = digitsum();
    printf("The sum of the digits of %d is equal to %d\n",
    num, sum);
    return;
}

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf(2, "Usage: factor number\n");
        exit();
    }
    else
    {
        sumOfDigits(argv[1]);
        exit();
    }
}