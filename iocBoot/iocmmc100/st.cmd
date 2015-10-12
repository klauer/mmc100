#!/bin/bash

# NOTE: the user that runs this IOC must have access to USB ports --
#       that could mean adding the user to the 'dialout' group, or however
#       else preferred
#
# A udev rule might look like the following:
#   $ cat /etc/udev/rules.d/99-mmc100.rules
#   # Bus 004 Device 005: ID 0403:6001 Future Technology Devices International, Ltd FT232 USB-Serial (UART) IC
#   KERNEL=="ttyUSB[0-9]*", SUBSYSTEMS=="usb", ATTRS{idVendor}=="0403", MODE="0666", GROUP=operator

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
