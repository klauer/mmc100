#!../../bin/linux-x86/mmc100test

< envPaths

## Register all support components
dbLoadDatabase("../../dbd/mmc100test.dbd",0,0)
mmc100test_registerRecordDeviceDriver(pdbbase) 

# Get the MMC100 port from the environment variable settings (see st.sh)
epicsEnvSet("MMC100_PORT", "$(MMC100_PORT)")

epicsEnvSet("PORT", "MMC100")
epicsEnvSet("ASYN_PORT", "$(ASYN_PORT=MMC100_ASYN)")

# Autosave prefix
epicsEnvSet("AS_PREFIX", "MMC100:AS:")

drvAsynSerialPortConfigure("$(ASYN_PORT)", "$(MMC100_PORT)", 0, 0, 0)
asynSetOption("$(ASYN_PORT)", -1,"baud",38400)
asynSetOption("$(ASYN_PORT)", -1,"bits",8)
asynSetOption("$(ASYN_PORT)", -1,"parity","none")
asynSetOption("$(ASYN_PORT)", -1,"stop",1)
asynSetOption("$(ASYN_PORT)", -1, "clocal", "N")
asynSetOption("$(ASYN_PORT)", -1, "crtscts", "N")

#MMC100CreateController(portName, MMC100PortName, numAxes, movingPollPeriod, idlePollPeriod)
MMC100CreateController("$(MMC100_PORT)", "$(ASYN_PORT)", 1, 50, 100)

#asynSetTraceMask("$(MMC100_PORT)", 0, 9)
#asynSetTraceMask("$(ASYN_PORT)", -1, 9)
#
#asynSetTraceIOMask("$(MMC100_PORT)", -1, 255)
#asynSetTraceIOMask("$(ASYN_PORT)", -1, 255)

# port_name
# axis_num
# enable            Disable (0) or enable (1) limit switches
# active_level      Active low (0) or active high (1)
#MMC100LimitSetup(portName, axis_num, enable, active_level)
MMC100LimitSetup("$(MMC100_PORT)", 0, 1, 1)

# port_name
# axis_num
# analog            Digital (0) or analog (1) encoder
# resolution        Encoder resolution (0.001 ~ 999.999 um/count or mdeg/count) (double)
# polarity          Normal (0) or reverse operation (1) (change if encoder pos seems wrong)
# deadband_counts   Continuous oscillation (0) or encoder counts (>=1) (int)
# deadband_timeout  Time to move into the deadband area (0.0 = infinite) (double)
MMC100EncoderSetup("$(MMC100_PORT)", 0, 0, 0.005, 1, 1, 0.0)

dbLoadTemplate("mmc100.sub")

cd $(TOP)/iocBoot/$(IOC)
< save_restore.cmd
iocInit()

create_monitor_set("auto_positions.req", 5, "P=$(AS_PREFIX)")
create_monitor_set("auto_settings.req", 30, "P=$(AS_PREFIX)")
