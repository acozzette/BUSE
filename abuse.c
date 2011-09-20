#include <assert.h>
#include <fcntl.h>
#include <linux/types.h>
#include <nbd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abuse.h"

int abuse_main(int argc, char *argv[], const struct abuse_operations *aop, void *userdata)
{
    int sp[2];
    int nbd, sk, err, tmp_fd;
    u_int64_t from;
    u_int32_t len;
    int bytes_written, bytes_read;
    char *dev_file;
    struct nbd_request request;
    struct nbd_reply reply;
    void *chunk;

    (void) userdata;

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

    reply.magic = htonl(NBD_REPLY_MAGIC);
    reply.error = htonl(0);

    while (1) {
        bytes_read = read(sk, &request, sizeof(request));
        assert(bytes_read == sizeof(request));
        memcpy(reply.handle, request.handle, sizeof(reply.handle));

        /* FIXME: these might need conversion from the network byte order. */
        len = ntohl(request.len);
        from = request.from;
        (void) from;
        assert(request.magic == htonl(NBD_REQUEST_MAGIC));

        switch(ntohl(request.type)) {
            case NBD_CMD_READ:
                chunk = malloc(len + sizeof(struct nbd_reply));
                aop->read((char *)chunk + sizeof(struct nbd_reply), len, from);
                memcpy(chunk, &reply, sizeof(struct nbd_reply));
                bytes_written = write(sk, chunk, len + sizeof(struct nbd_reply));
                fprintf(stderr, "Wrote %d bytes.\n", bytes_written);
                /* assert(bytes_written == len + sizeof(struct nbd_reply)); */
                free(chunk);
                break;
            default:
                /* We'll not worry about the other cases for now. */
                break;
        }
    }

    return 0;
}
