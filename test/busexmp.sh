#!/usr/bin/env bash
set -e
source ./test/utils.sh

BLOCKDEV=/dev/nbd0
# quiet version of dd
DD="dd status=none"

ensure_buse_device_free "$BLOCKDEV"

# on exit do cleanup actions
function cleanup () {
	# disconnect and wait for BUSE background job
	disconnect_buse_device "$BLOCKDEV"
	wait $BUSEPID
	# remove the test file
	rm -f "$TESTFILE"
}
trap cleanup EXIT

# prepare file with some data
TESTFILE=$(mktemp)
$DD if=/dev/urandom of="$TESTFILE" bs=16M count=1

# attach BUSE device
busexmp 128M "$BLOCKDEV" &
sleep 1
BUSEPID=$!

### do checks ###

# initialy there are all zeros
cmp <($DD if="$BLOCKDEV" ibs=1k count=5 skip=54) <($DD if=/dev/zero bs=1k count=5)

# write data at the end
$DD if="$TESTFILE" of="$BLOCKDEV" bs=1M count=2 seek=126

# read extending past the end of device
cmp <($DD if="$BLOCKDEV" bs=1M count=5 skip=126) <($DD if="$TESTFILE" bs=1M count=2)
