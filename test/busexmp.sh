#!/usr/bin/env bash
set -e

BLOCKDEV=/dev/nbd0
# quiet version of dd
DD="dd status=none"

# verify if blockdev is not currently in use
set +e
nbd-client -c "$BLOCKDEV" > /dev/null
if [ $? -ne 1 ]; then
	echo "device $BLOCKDEV is not ready to use (already in use or corrupted)"
	exit 1
fi
set -e

# on exit do cleanup actions
function cleanup () {
	# kill and wait for BUSE background job
	nbd-client -d "$BLOCKDEV" > /dev/null
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
