#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

void testDebuggingChild()
{
    int pid = getpid();
    int parentpid = getparentpid();
    printf(1, "Process id: %d Process' parent id: %d\n", pid, parentpid);
    return;
}

int main(int argc, char* argv[])
{
    testDebuggingChild();
    sleep(500);
    testDebuggingChild();
    exit();
}