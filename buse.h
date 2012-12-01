#ifndef BUSE_H_INCLUDED
#define BUSE_H_INCLUDED

/* Most of this file was copied from nbd.h in the nbd distribution. */

#include <linux/types.h>
#include <sys/types.h>

#define NBD_SET_SOCK	_IO( 0xab, 0 )
#define NBD_SET_BLKSIZE	_IO( 0xab, 1 )
#define NBD_SET_SIZE	_IO( 0xab, 2 )
#define NBD_DO_IT	_IO( 0xab, 3 )
#define NBD_CLEAR_SOCK	_IO( 0xab, 4 )
#define NBD_CLEAR_QUE	_IO( 0xab, 5 )
#define NBD_PRINT_DEBUG	_IO( 0xab, 6 )
#define NBD_SET_SIZE_BLOCKS	_IO( 0xab, 7 )
#define NBD_DISCONNECT  _IO( 0xab, 8 )
#define NBD_SET_TIMEOUT _IO( 0xab, 9 )
#define NBD_SET_FLAGS _IO( 0xab, 10 )

enum {
	NBD_CMD_READ = 0,
	NBD_CMD_WRITE = 1,
	NBD_CMD_DISC = 2,
	NBD_CMD_FLUSH = 3,
	NBD_CMD_TRIM = 4
};

/* values for flags field */
#define NBD_FLAG_HAS_FLAGS	(1 << 0)
#define NBD_FLAG_READ_ONLY	(1 << 1)
/* there is a gap here to match userspace */
#define NBD_FLAG_SEND_TRIM	(1 << 5) /* send trim/discard */

/* Magic numbers */
#define NBD_REQUEST_MAGIC 0x25609513
#define NBD_REPLY_MAGIC 0x67446698

/*
 * This is the packet used for communication between client and
 * server. All data are in network byte order.
 */
struct nbd_request {
	__be32 magic;
	__be32 type;	/* == READ || == WRITE 	*/
	char handle[8];
	__be64 from;
	__be32 len;
} __attribute__ ((packed));

/*
 * This is the reply packet that nbd-server sends back to the client after
 * it has completed an I/O request (or an error occurs).
 */
struct nbd_reply {
	__be32 magic;
	__be32 error;		/* 0 = ok, else error	*/
	char handle[8];		/* handle you got from request	*/
};

struct buse_operations {
    int (*read)(void *buf, u_int32_t len, u_int64_t offset);
    int (*write)(const void *buf, u_int32_t len, u_int64_t offset);
    void (*disc)();
    int (*flush)();
    int (*trim)(u_int64_t from, u_int32_t len);

    u_int64_t size;
};

int buse_main(int argc, char *argv[], const struct buse_operations *bop,
        void *userdata);

#endif /* BUSE_H_INCLUDED */
