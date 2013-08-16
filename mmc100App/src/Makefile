TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += mmc100

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install mmc100.dbd into <top>/dbd
DBD += mmc100.dbd

# specify all source files to be compiled and added to the library
mmc100_SRCS += main.cpp MMC100.cpp 
#mmc100_SRCS += $(MOTOR)/motorApp/MotorSrc/asynMotorController.cpp
#mmc100_SRCS += $(MOTOR)/motorApp/MotorSrc/asynMotorAxis.cpp
mmc100_SRCS += asynMotorController.cpp
mmc100_SRCS += asynMotorAxis.cpp

mc100_LIBS += motor asyn
mc100_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

