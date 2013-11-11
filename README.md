PI MiCos MMC100 EPICS Driver
============================

Motor record driver ("model-3" type) for the PI MiCos MMC100 piezo controller.

USB Notes
---------

If using the MMC100 with a USB cable to a Linux computer, the user that runs this IOC
must obviously have access to USB ports -- this could mean adding the user to the 'dialout'
group, adding udev rules, or however the specific Linux distribution does it.

As it is, the default st.cmd is actually a bash script that looks for a specific vendor/product ID
by using a simple HAL script. Used in conjunction with procServ, the IOC will die when the device
is unplugged, but restart automatically when it is detected again.

An easier, more permanent method, may be to set SYMLINK+= in your udev rules such that 
/dev/mmc100 is created, for example. (TODO examples)

Requirements
------------

Though it may work on other versions, the driver was tested on these:

1. EPICS base 3.14.12.3 http://www.aps.anl.gov/epics/
2. asyn 4-18 http://www.aps.anl.gov/epics/modules/soft/asyn/
3. motor record 6-7 http://www.aps.anl.gov/bcda/synApps/motor/

Optional
--------

1. EDM http://ics-web.sns.ornl.gov/edm/log/getLatest.php
2. Autosave http://www.aps.anl.gov/bcda/synApps/autosave/autosave.html

Installation
------------

1. Install EPICS
    1. If using a Debian-based system (e.g., Ubuntu), use the packages here http://epics.nsls2.bnl.gov/debian/
    2. If no packages are available for your distribution, build from source
2. Edit configure/RELEASE
    1. Point the directories listed in there to the appropriate places
    2. If using the Debian packages, everything can be pointed to /usr/lib/epics
3. Edit iocBoot/iocmmc100test/st.cmd
    1. Change the shebang on the top of the script if your architecture is different than linux-x86:
        #!../../bin/linux-x86/mmc100test
        (check if the environment variable EPICS_HOST_ARCH is set, or perhaps `uname -a`, or ask someone if
         you don't know)
    2. The following line sets the autosave prefix for your MMC100 PVs:
        ```
        # Autosave prefix
        epicsEnvSet("AS_PREFIX", "MMC100:AS:")
        ```
       Set the second quoted string appropriately.
    3. If the find_usb.sh script is being used, the $MMC100_PORT environment variable will be automatically set. 
        If the port which the MMC100 is connected to is fixed, you can set it in place of $(MMC100_PORT), on the line
        `epicsEnvSet("MMC100_PORT", "$(MMC100_PORT)")` -- and st1.cmd can then replace st.cmd. 
    4. If necessary, you can change the rate at which the controller is polled for positions and such:
        ```
        #MMC100CreateController(portName, MMC100PortName, numAxes, movingPollPeriod, idlePollPeriod)
        MMC100CreateController("$(MMC100_PORT)", "$(ASYN_PORT)", 1, 50, 100)
        ```
        The moving and idle poll periods are both in milliseconds. The former rate is used when an axis is in motion, the latter otherwise.
    5. Set-up the limits and encoder settings for each axis on the controllers (see the example in the file).
    6.  If using autosave, add lines in auto_positions.req and auto_settings.req for each motor. If not using autosave, comment create_monitor_set lines.
4.  Edit iocBoot/iocMMC100/MMC100.sub
    Modify the lines so that there is one motor.db line per axis. Each will be named $(P)$(M).
5. Go to the top directory and `make`
6. If all goes well:
    ```
    $ cd iocBoot/iocmmc100
    $ chmod +x st.cmd
    $ ./st.cmd

    If not using the find_usb.sh script, run ./st1.cmd directly:
    $ ./st1.cmd
    ```

7. Run EDM:
    ```
    $ export EDMDATAFILES=$TOP/op/edl:$EDMDATAFILES
    $ edm -x -m "P=MLL:,M=m1" motorx_all &
    ```
