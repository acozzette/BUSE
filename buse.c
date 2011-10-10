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

#include "buse.h"

/*
 * These helper functions were taken from cliserv.h in the nbd distribution.
 */
#ifdef WORDS_BIGENDIAN
u_int64_t ntohll(u_int64_t a) {
	return a;
}
#else
u_int64_t ntohll(u_int64_t a) {
	u_int32_t lo = a & 0xffffffff;
	u_int32_t hi = a >> 32U;
	lo = ntohl(lo);
	hi = ntohl(hi);
	return ((u_int64_t) lo) << 32U | hi;
}
#endif
#define htonll ntohll

int buse_main(int argc, char *argv[], const struct buse_operations *aop, void *userdata)
{
    int sp[2];
    int nbd, sk, err, tmp_fd;
    u_int64_t from;
    u_int32_t len, bytes_read, bytes_written;
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

        len = ntohl(request.len);
        from = ntohll(request.from);
        assert(request.magic == htonl(NBD_REQUEST_MAGIC));

        switch(ntohl(request.type)) {
            /* I may at some point need to deal with the the fact that the
             * official nbd server has a maximum buffer size, and divides up
             * oversized requests into multiple pieces. This applies to reads
             * and writes.
             */
            case NBD_CMD_READ:
                chunk = malloc(len + sizeof(struct nbd_reply));
                aop->read((char *)chunk + sizeof(struct nbd_reply), len, from);
                memcpy(chunk, &reply, sizeof(struct nbd_reply));
                bytes_written = write(sk, chunk, len + sizeof(struct nbd_reply));
                assert(bytes_written == len + sizeof(struct nbd_reply));
                fprintf(stderr, "Wrote %d bytes.\n", bytes_written);
                free(chunk);
                break;
            case NBD_CMD_WRITE:
                chunk = malloc(len);
                bytes_read = read(sk, &chunk, len);
                assert(bytes_read == len);
                aop->write(chunk, len, from);
                free(chunk);
                bytes_written = write(sk, &reply, sizeof(struct nbd_reply));
                assert(bytes_written == sizeof(struct nbd_reply));
                break;
            case NBD_CMD_DISC:
                aop->disc();
                break;
            case NBD_CMD_FLUSH:
                aop->flush();
                break;
            case NBD_CMD_TRIM:
                aop->trim();
                break;
            default:
                assert(0);
        }
    }

    return 0;
}
