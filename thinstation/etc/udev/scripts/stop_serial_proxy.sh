#!/bin/sh

. /etc/thinstation.env
. $TS_GLOBAL
. $TS_NETWORK

if [ "$SERIAL_PROXY_DEVICE" ] && [ "$DEVPATH" ] && [ -d /sys/"$DEVPATH"/`basename "$SERIAL_PROXY_DEVICE"` ] ; then
        logger "Forcibly stopping serial_proxy because of usbserial dongle $SERIAL_PROXY_DEVICE $ACTION"
        /bin/killall serial_proxy
fi
