#!../../bin/linux-x86_64/mmc100test

epicsEnvSet("ENGINEER",  "klauer x3615")
epicsEnvSet("LOCATION",  "HXN C-hutch RG:I5")

epicsEnvSet("SYS",           "XF:03IDC-ES")
epicsEnvSet("IOC_PREFIX",    "$(SYS){IOC:$(IOCNAME)}")

epicsEnvSet("EPICS_CA_AUTO_ADDR_LIST", "NO")
epicsEnvSet("EPICS_CA_ADDR_LIST", "10.3.0.255")

< envPaths

## Register all support components
dbLoadDatabase("../../dbd/mmc100test.dbd",0,0)
mmc100test_registerRecordDeviceDriver(pdbbase) 

# Get the MMC100 port from the environment variable settings (see st.cmd)
# epicsEnvSet("MMC100_PORT", "MMC_1")

epicsEnvSet("PORT", "MMC100")
epicsEnvSet("ASYN_PORT", "$(ASYN_PORT=MMC100_ASYN)")

# Autosave prefix
epicsEnvSet("AS_PREFIX", "MMC100:AS:")

drvAsynSerialPortConfigure("$(ASYN_PORT)", "$(MMC100_PORT)", 0, 0, 0)
asynSetOption("$(ASYN_PORT)", -1,"baud", 38400)
asynSetOption("$(ASYN_PORT)", -1,"bits", 8)
asynSetOption("$(ASYN_PORT)", -1,"parity", "none")
asynSetOption("$(ASYN_PORT)", -1,"stop", 1)
asynSetOption("$(ASYN_PORT)", -1, "clocal", "N")
asynSetOption("$(ASYN_PORT)", -1, "crtscts", "N")

#MMC100CreateController(portName, MMC100PortName, numAxes, movingPollPeriod, idlePollPeriod)
MMC100CreateController("MMC_1", "$(ASYN_PORT)", 4, 50, 100)

#asynSetTraceMask("MMC_1", 0, 9)
#asynSetTraceMask("$(ASYN_PORT)", -1, 9)
#
#asynSetTraceIOMask("MMC_0", -1, 255)
#asynSetTraceIOMask("$(ASYN_PORT)", -1, 255)

# port_name
# axis_num
# enable            Disable (0) or enable (1) limit switches
# active_level      Active low (0) or active high (1)
#MMC100LimitSetup(portName, axis_num, enable, active_level)
MMC100LimitSetup("MMC_1", 0, 1, 1)
MMC100LimitSetup("MMC_1", 1, 1, 1)
MMC100LimitSetup("MMC_1", 2, 1, 1)
MMC100LimitSetup("MMC_1", 3, 1, 1)

# port_name
# axis_num
# analog            Digital (0) or analog (1) encoder
# resolution        Encoder resolution (0.001 ~ 999.999 um/count or mdeg/count) (double)
# polarity          Normal (0) or reverse operation (1) (change if encoder pos seems wrong)
# deadband_counts   Continuous oscillation (0) or encoder counts (>=1) (int)
# deadband_timeout  Time to move into the deadband area (0.0 = infinite) (double)
# feedback          0 open-loop 3 closed-loop
MMC100EncoderSetup("MMC_1", 0, 0, 0.005, 1, 500, 5.0, 3)
MMC100EncoderSetup("MMC_1", 1, 0, 0.005, 1, 500, 10.0, 3)
MMC100EncoderSetup("MMC_1", 2, 0, 0.005, 1, 500, 5.0, 3)
MMC100EncoderSetup("MMC_1", 3, 0, 0.005, 1, 500, 0.0, 3)

dbLoadTemplate("mmc100.sub")

cd $(TOP)/iocBoot/$(IOC)

## autosave/restore machinery
save_restoreSet_Debug(0)
save_restoreSet_IncompleteSetsOk(1)
save_restoreSet_DatedBackupFiles(1)

set_savefile_path("${TOP}/as","/save")
set_requestfile_path("${TOP}/as","/req")
system("install -m 777 -d ${TOP}/as/save")
system("install -m 777 -d ${TOP}/as/req")

set_pass0_restoreFile("info_positions.sav")
set_pass0_restoreFile("info_settings.sav")
set_pass1_restoreFile("info_settings.sav")

dbLoadRecords("$(EPICS_BASE)/db/save_restoreStatus.db","P=$(IOC_PREFIX)")
dbLoadRecords("$(EPICS_BASE)/db/iocAdminSoft.db","IOC=$(IOC_PREFIX)")
save_restoreSet_status_prefix("$(IOC_PREFIX)")
iocInit()

# caPutLogInit("ioclog.cs.nsls2.local:7004", 1)

## more autosave/restore machinery
cd ${TOP}/as/req
makeAutosaveFiles()
create_monitor_set("info_positions.req", 5 , "")
create_monitor_set("info_settings.req", 15 , "")

cd ${TOP}
dbl > ./records.dbl
system "cp ./records.dbl /cf-update/$HOSTNAME.$IOCNAME.dbl"
