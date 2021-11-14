#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int writeIds(int pid, int parentpid)
{
    int fd = open("testdebuggingoutput.txt", O_RDWR | O_CREATE | O_TRUNC);
    if(fd < 0)
    {
        return -1;
    }
    printf(fd, "%d %d\n", pid, parentpid);
    close(fd);
    return 0;
}

void testDebuggingChild()
{
    int pid = getpid();
    int parentpid = getparentpid();
    printf(1, "Process id: %d Process' parent id: %d\n", pid, parentpid);
    if(writeIds(pid, parentpid) < 0)
    {
        printf(2, "Debugging child write in file error\n");
        exit();
    }
    return;
}

int main(int argc, char* argv[])
{
    testDebuggingChild();
    sleep(500);
    testDebuggingChild();
    exit();
}