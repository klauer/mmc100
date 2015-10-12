#!/bin/bash

# NOTE: the user that runs this IOC must have access to USB ports --
#       that could mean adding the user to the 'dialout' group, or however
#       else preferred

# find the ttyUSB port where the MMC100 is connected:
MMC100_PORT=`./find_usb.sh 0403 6001 2> /dev/null`
export MMC100_PORT

if [ -n "$MMC100_PORT" ]
then
    echo "----> MMC100 port is ${MMC100_PORT}"
    ./st1.cmd
else
    echo "----> MMC100 not detected"
    sleep 1
fi
