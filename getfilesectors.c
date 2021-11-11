#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"


void getFileSectors(char* argv)
{
    int fd = atoi(argv);
    int address[NDIRECT + 1];
    int size;
    if(getfilesectors(fd, address, &size) < 0)
    {
        printf(2, "Usage: get file sectors file problem\n");
        return;
    }
    printf(1, "The sectors for %d are:", fd);
    for(int i = 0; i < size; i++)
        printf(1, "%d ", address[i]);
    printf(1, "\n");
    return;
}

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf(2, "Usage: get file sectors number\n");
        exit();
    }
    else
    {
        getFileSectors(argv[1]);
        exit();
    }
}