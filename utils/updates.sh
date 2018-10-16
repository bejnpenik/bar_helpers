#!/bin/sh
getupdates() {
	if ! updates=$(checkupdates 2> /dev/null | wc -l ); then
    		updates=0
	fi

	if [ "$updates" -gt 0 ]; then
    		echo "UPD;$updates"
	else
    		echo "UPD;0"
	fi
}

while true; do
	getupdates
	sleep 60
done
