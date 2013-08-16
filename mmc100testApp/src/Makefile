TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = mmc100test
# mmc100test.dbd will be created and installed
DBD += mmc100test.dbd

# mmc100test.dbd will be made up from these files:
mmc100test_DBD += base.dbd

# Include dbd files from all support applications:
mmc100test_DBD += drvAsynSerialPort.dbd motorRecord.dbd motorSupport.dbd asyn.dbd mmc100.dbd asSupport.dbd

# Add all the support libraries needed by this IOC
mmc100test_LIBS += asyn mmc100 motor autosave

# mmc100test_registerRecordDeviceDriver.cpp derives from mmc100test.dbd
mmc100test_SRCS += mmc100test_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
mmc100test_SRCS_DEFAULT += mmc100testMain.cpp
mmc100test_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#mmc100test_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
mmc100test_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

