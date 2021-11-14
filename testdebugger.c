#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

void testdebugger()
{
    int fd = open("testdebuggingoutput.txt", O_RDONLY);
    if(fd < -1)
    {
        printf(2, "Test debugging file open problem\n");
        return;
    }
    char strPid[256];
    if(read(fd, strPid, 4) < 0)
    {
        printf(2, "Test debugging file read problem\n");
        return;
    }
    int pid = atoi(strPid);
    if(setprocessparent(pid) < 0)
    {
        printf(2, "Set process parent error\n");
        return;
    }
    printf(1, "Test debugging done\n");
    return;
}

int main(int argc, char* argv[])
{
    testdebugger();
    exit();
}