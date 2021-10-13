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
        x /= 10;
    }
    return digits;
}
char* intToString(int x)
{
    int wordSize = getNumOfDigits(x);
    printf(1, "%d digits %d\n", x, wordSize);
    wordSize += 2;
    char* str = (char*) malloc(wordSize);
    str[wordSize - 1] = '\0';
    str[wordSize - 2] = '\n';

    for(int i = wordSize - 3; i >= 0; i--)
    {
        str[i] = (x % 10) + '0';
        x /= 10;
    }
    printf(1, "%s\n", str); 
    return str;
}



void factor(char* argv)
{
    int n = atoi(argv);
    int fd = open(OUTPUT_FILE, O_WRONLY | O_CREATE);
    //write(fd, "hello", sizeof("hello"));
    //return;
    printf(1, "fd %d\n", fd);
    if(fd < 0)
    {
        printf(2, "factor cannot open %s\n", OUTPUT_FILE);
        return;
    }
    
    
    for(int i = 1; i * i <= n ; i++)
    {
        if(n % i == 0)
        {
            //printf(1, "%d\n", i);
            char* str = intToString(i);
            if(write(fd, str, sizeof(*str)) != sizeof(*str))
            {
                printf(2, "write error\n");
                return;
            }
            free(str);
            if(i != n/i)
            {
                //printf(1, "%d\n", n/i);
                str = intToString(n/i);
                if(write(fd, str, sizeof(*str))!= sizeof(*str))
                {
                    printf(2, "write error\n");
                    return;
                }
                free(str);
            }
        }
    }
    //close(fd);
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