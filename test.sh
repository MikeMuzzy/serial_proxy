#!/bin/sh
#./agent 127.0.0.1:4767
#./agent 192.168.77.31:4767
make clean
make
./main detect ice5000 /dev/ttyS0
sleep 1
./main detect ice5000 /dev/ttyS0
sleep 1
./main detect ice5000 /dev/ttyS0
sleep 1
./main detect ice5000 /dev/ttyS0
cat /tmp/serial_proxy.log
