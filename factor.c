#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

const char OUTPUT_FILE[] = "factor_result.txt";

int getNumOfDigits(int x)
{
    if(x < 10)
        return 1;
    int digits = 0;
    while(x > 0)
    {
        digits++;
        x = x % 10;
    }
    return digits;
}
char* intToString(int x)
{
    int digits = getNumOfDigits(x);
    char* str = (char*) malloc(digits);
    
    for(int i = digits - 1; i >= 0; i--)
    {
        str[i] = x % 10;
        x /= 10;
    }
    return str;
}

int writeNumberInFile(int fd, int x, char* str)
{
    int written = write(fd, str, sizeof(str));
    free(str);
    return written;
}

void factor(char* argv)
{
    int n = atoi(argv);
    int fd = open(OUTPUT_FILE, O_WRONLY | O_CREATE);
    if(fd < 0)
    {
        printf(2, "factor cannot open %s\n", OUTPUT_FILE);
        return;
    }
    
    
    for(int i = 1; i * i <= n ; i++)
    {
        if(n % i == 0)
        {
            printf(1, "%d\n", i);
            char* str = intToString(i);
            if(writeNumberInFile(fd, i, str) != sizeof(str))
            {
                printf(2, "write error\n");
                return;
            }
            if(i != n/i)
            {
                printf(1, "%d\n", n/i);
                str = intToString(n/i);
                if(writeNumberInFile(fd, n/i, str) != sizeof(str))
                {
                    printf(2, "write error\n");
                    return;
                }
            }
        }
    }
    close(fd);
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
        factor(argv[1]);
        exit();
    }
}