#!/usr/bin/env bash
set -e

BLOCKDEV=/dev/nbd0
SIZE=16M

# verify if blockdev is not currently in use
set +e
nbd-client -c "$BLOCKDEV" > /dev/null
if [ $? -ne 1 ]; then
	echo "device $BLOCKDEV is not ready to use (already in use or corrupted)"
	exit 1
fi
set -e

# on exit make sure nbd is disconnected
function cleanup () {
	nbd-client -d "$BLOCKDEV" > /dev/null
}
trap cleanup EXIT

# attach BUSE device
busexmp "$SIZE" "$BLOCKDEV" &
BUSEPID=$!

# wait a bit ensure buse is running and connected to device
sleep 1
nbd-client -c "$BLOCKDEV" > /dev/null

# kill it with SIGTERM. Exit code should be 0 - if not bash will break because we have -e option.
kill -s SIGTERM $BUSEPID
wait $BUSEPID

# attach BUSE again to verify if device is left in usable state
busexmp "$SIZE" "$BLOCKDEV" &
BUSEPID=$!
sleep 1
nbd-client -c "$BLOCKDEV" > /dev/null
kill -s SIGINT $BUSEPID # this time kill it with SIGINT
wait $BUSEPID
