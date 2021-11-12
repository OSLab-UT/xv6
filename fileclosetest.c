#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

void fileclosetest()
{
    for(int fd = 3; fd <= 6; fd++)
        close(fd);
    return;
}

int main(int argc, char* argv[])
{
    fileclosetest();
    exit();
}