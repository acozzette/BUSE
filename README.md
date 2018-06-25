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

BUSE comes with an example driver in `busexmp.c` that implements a
memory disk. To try out the example code, run `make` and then execute the
following as root:

    modprobe nbd
    ./busexmp 128M /dev/nbd0

You should then have an in-memory disk running, represented by the device file
`/dev/nbd0`. You can create a file system on the virtual disk, mount it, and
start reading and writing files on it:

    mkfs.ext4 /dev/nbd0
    mount /dev/nbd0 /mnt

BUSE should gracefuly disconnect from block device upon receiving SIGINT
or SIGTERM. However, if something goes wrong, block device is stuck in
unusable state and BUSE process exited or hung you can request
disconnect by:

    nbd-client -d /dev/nbd0

Actually this command performs clean disconnect and can also be used
to terminate running instance of BUSE.

## Tests

To perform checks you can run scripts in `test/` directory. They require:
 * superuser previlages,
 * nbd kernel module loaded,
 * BUSE and nbd (`nbd-client`) binaries in PATH.

`make test` will run all test scripts with BUSE added to PATH and using
sudo to grant permissions.

To increase verbosity define `BUSE_DEBUG`. You can do this in make command:

    make test CFLAGS=-DBUSE_DEBUG
