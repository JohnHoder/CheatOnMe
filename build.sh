pebble build || { exit $?; }
if [ "$1" = "install" ]; then
	pebble install --phone 192.168.0.3 --logs
fi
