#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    //不需要多余参数
    if (argc != 1)
    {
        printf("sysinfo need no param\n");
        exit(1);
    }

    struct sysinfo info;
    sysinfo(&info);
    printf("free space: %d\nprocess: %d\nfree fd: %d\n", info.freemem, info.nproc, info.freefd);
    exit(0);
}

