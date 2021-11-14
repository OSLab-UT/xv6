#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

void testdebugger(char* argv)
{
    int pid = atoi(argv);
    if(setprocessparent(pid) < 0)
    {
        printf(2, "Set process parent error\n");
    }
    printf(1, "Test debugging done\n");
    return;
}

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf(2, "Usage: debugger input\n");
        exit();
    }
    else
    {
        testdebugger(argv[1]);
        exit();
    }
}