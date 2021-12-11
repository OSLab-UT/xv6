#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

void setMHRRNprocessSpace(char* argv)
{
    int MHRRN = atoi(argv);
    if(setMHRRNprocessspace(MHRRN) < 0)
    {
        printf(2, "Usage: setMHRRNprocess system call\n");
        exit();
    }
    return;
}

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf(2, "Usage: setMHRRNprocess input\n");
        exit();
    }
    else
    {
        setMHRRNprocessspace(argv[1]);
        exit();
    }
}