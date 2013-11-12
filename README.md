# BUSE - A block device in userspace

This piece of software was inspired by FUSE, which allows the development of
Linux file systems that run in userspace. The goal of BUSE is to allow virtual
block devices to run in userspace as well. Currently BUSE is experimental and
should not be used for production code.

Implementing a block device with BUSE is fairly straightforward. Simply fill
`struct buse_operations` (declared in `buse.h`) with function pointers that
define the behavior of the block device, and set the size field to be the
desired size of the device in bytes. Then call `buse_main` and pass it a
pointer to this struct. `busexmp.c` is a simple example example that shows how
this is done.

The implementation of BUSE itself relies on NBD, the Linux network block device,
which allows a remote machine to serve requests for reads and writes to a
virtual block device on the local machine. BUSE sets up an NBD server and client
on the same machine, with the server executing the code defined by the BUSE
user.

## Running the Example Code

BUSE comes with an example driver in `busexmp.c` that implements a 128 MB
memory disk. To try out the example code, run `make` and then execute the
following as root:

    modprobe nbd
    ./busexmp /dev/nbd0

You should then have an in-memory disk running, represented by the device file
`/dev/nbd0`. You can create a file system on the virtual disk, mount it, and
start reading and writing files on it:

    mkfs.ext4 /dev/nbd0
    mount /dev/nbd0 /mnt
