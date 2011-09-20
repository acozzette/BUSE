#include <assert.h>
#include <fcntl.h>
#include <linux/types.h>
#include <nbd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abuse.h"

int abuse_main(int argc, char *argv[], struct abuse_operations *aop)
{
    int sp[2];
    int nbd, sk, err, tmp_fd;
    char *dev_file;

    assert(argc == 2);
    dev_file = argv[1];

    assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, sp));

    nbd = open(dev_file, O_RDWR);
    assert(nbd != -1);

    assert(ioctl(nbd, NBD_SET_SIZE, aop->size) != -1);
    assert(ioctl(nbd, NBD_CLEAR_SOCK) != -1);

    if (!fork()) {
        /* The child needs to continue setting things up. */
        close(sp[0]);
        sk = sp[1];

        assert(ioctl(nbd, NBD_SET_SOCK, sk) != -1);
        err = ioctl(nbd, NBD_DO_IT);
        fprintf(stderr, "nbd device terminated with code %d\n", err);

        assert(ioctl(nbd, NBD_CLEAR_QUE) != -1);
        assert(ioctl(nbd, NBD_CLEAR_SOCK) != -1);

        exit(0);
    }

    /* The parent opens the device file at least once, to make sure the
     * partition table is updated. Then it closes it and starts serving up
     * requests. */

    tmp_fd = open(dev_file, O_RDONLY);
    assert(tmp_fd != -1);
    close(tmp_fd);

    close(sp[1]);
    sk = sp[0];

    return 0;
}
