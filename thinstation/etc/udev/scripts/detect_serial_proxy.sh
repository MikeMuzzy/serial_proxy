#!/bin/sh

. /etc/thinstation.env
. $TS_GLOBAL

if [ -z "$DEVNAME" ]; then
    device=$1
else
    device=$DEVNAME
fi

mkdir -p /dev/cashhw/

while [ `ps |  grep -v grep | grep -c ntpdate` != "0" ]; do
    sleep 1
done

dev_full_type=`/bin/serial_proxy detect $device`
if [ "$?" = "0" ]; then
    dev_type=`echo $dev_full_type | awk -F: '{print $1}'`
    case $dev_type in
	maria301 | datecs)
	    ln -f -s $device /dev/cashhw/fp
	    ln -f -s $device /dev/fp
	    [ -z "$SERIAL_PROXY_DEVICE" ] || [ "$SERIAL_PROXY_DEVICE" = "/dev/fp" ] || [ "$SERIAL_PROXY_DEVICE" = "auto" ] && {
		echo "device /dev/cashhw/fp" > $SERIAL_PROXY_CONFIG
	    }
	;;
	privat)
	    ln -f -s $device /dev/cashhw/term_privat
	;;
	fuib)
	    ln -f -s $device /dev/cashhw/term_pumb
	;;
    esac
fi
