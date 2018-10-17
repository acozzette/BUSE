function ensure_buse_device_free () {
	# verify if blockdev is not currently in use
	set +e
	nbd-client -c "$1" > /dev/null
	if [ $? -ne 1 ]; then
		echo "device $1 is not ready to use (already in use or corrupted)"
		exit 1
	fi
	set -e
}

function disconnect_buse_device () {
	nbd-client -d "$1" > /dev/null
}
