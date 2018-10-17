#!/usr/bin/env bash
set -e
source ./test/utils.sh

BLOCKDEV=/dev/nbd0
FAKE_BLOCK_DEVICE=/dev/fake-dev0
# quiet version of dd
DD="dd status=none"

ensure_buse_device_free "$BLOCKDEV"

# ensure FAKE_BLOCK_DEVICE is not in use
set +e
losetup "$FAKE_BLOCK_DEVICE" 2> /dev/null
if [ $? -ne 1 ]; then
	echo "device $FAKE_BLOCK_DEVICE is not ready to use (already in use or corrupted)"
	exit 1
fi
set -e

# on exit do cleanup actions
function cleanup () {
	# kill and wait for BUSE background job
	disconnect_buse_device "$BLOCKDEV"
	wait $BUSEPID
	# remove fake block device
	losetup -d "$FAKE_BLOCK_DEVICE"
	rm -f "$FAKE_BLOCK_DEVICE"
	# remove the test file
	rm -f "$TESTFILE"
}
trap cleanup EXIT

# prepare fake block device
TESTFILE=$(mktemp)
$DD if=/dev/urandom of="$TESTFILE" bs=16M count=1
mknod "$FAKE_BLOCK_DEVICE" b 7 200 
losetup "$FAKE_BLOCK_DEVICE" "$TESTFILE"

# attach BUSE device
loopback "$FAKE_BLOCK_DEVICE" "$BLOCKDEV" &
sleep 1
BUSEPID=$!

### do checks ###

# compare data read from nbd with TESTFILE contents
cmp <($DD if="$BLOCKDEV" ibs=1k count=5 skip=54) <($DD if="$TESTFILE" ibs=1k count=5 skip=54)

# write data at the end
$DD if="$TESTFILE" of="$BLOCKDEV" bs=1M count=2 seek=14

# read extending past the end of device
cmp <($DD if="$BLOCKDEV" bs=1M count=5 skip=14) <($DD if="$TESTFILE" bs=1M count=2)
