/*
 * Micos MMC-100 EPICS Driver
 * Ken Lauer (klauer@bnl.gov)
 * Brookhaven National Laboratory
 * */

#include "MMC100.h"

static ELLLIST MMC100List;
static int MMC100ListInitialized = 0;

bool addToList(const char *portName, MMC100Controller *drv) {
    if (!MMC100ListInitialized) {
        MMC100ListInitialized = 1;
        ellInit(&MMC100List);
    } else if (findByPortName(portName) != NULL) {
        fprintf(stderr, "ERROR: Re-using portName=%s\n", portName);
        return false;
    }

    MMC100Node *pNode = (MMC100Node*)calloc(1, sizeof(MMC100Node));
    pNode->portName = epicsStrDup(portName);
    pNode->pController = drv;
    ellAdd(&MMC100List, (ELLNODE*)pNode);
    return true;
}

MMC100Controller* findByPortName(const char *portName) {
    MMC100Node *pNode;
    static const char *functionName = "findByPortName";

    // Find this 
    if (!MMC100ListInitialized) {
        printf("%s:%s: ERROR, MMC100 list not initialized\n",
            driverName, functionName);
        return NULL;
    }

    pNode = (MMC100Node*)ellFirst(&MMC100List);
    while(pNode) {
        if (!strcmp(pNode->portName, portName)) {
            return pNode->pController;
        }
        pNode = (MMC100Node*)ellNext((ELLNODE*)pNode);
    }

    printf("%s: MMC100 on port %s not found\n",
        driverName, portName);
    return NULL;
}

///// MMC100CreateController
//
/** Creates a new MMC100Controller object.
  * Configuration command, called directly or from iocsh
  * \param[in] type              The type of the controller [Use GCS for fully GCS-compatible controllers] (GCS, E-755, ...)
  * \param[in] portName          The name of the asyn port that will be created for this driver
  * \param[in] MMC100PortName     The name of the drvAsynIPPPort that was created previously to connect to the MMC100 controller 
  * \param[in] numAxes           The number of axes that this controller supports 
  * \param[in] movingPollPeriod  The time in ms between polls when any axis is moving
  * \param[in] idlePollPeriod    The time in ms between polls when no axis is moving 
  */
extern "C" int MMC100CreateController(const char *portName, const char *MMC100PortName, int numAxes, 
                                   int movingPollPeriod, int idlePollPeriod)
{
  new MMC100Controller(portName, MMC100PortName, numAxes, movingPollPeriod/1000., idlePollPeriod/1000.);
  return(asynSuccess);
}


/** Code for iocsh registration */
static const iocshArg MMC100CreateControllerArg0 = {"Port name", iocshArgString};
static const iocshArg MMC100CreateControllerArg1 = {"MMC100 port name", iocshArgString};
static const iocshArg MMC100CreateControllerArg2 = {"Number of axes", iocshArgInt};
static const iocshArg MMC100CreateControllerArg3 = {"Moving poll period (ms)", iocshArgInt};
static const iocshArg MMC100CreateControllerArg4 = {"Idle poll period (ms)", iocshArgInt};
static const iocshArg * const MMC100CreateControllerArgs[] = {&MMC100CreateControllerArg0,
                                                               &MMC100CreateControllerArg1,
                                                               &MMC100CreateControllerArg2,
                                                               &MMC100CreateControllerArg3,
                                                               &MMC100CreateControllerArg4
                                                            };
static const iocshFuncDef MMC100CreateControllerDef = {"MMC100CreateController", 5, MMC100CreateControllerArgs};
static void MMC100CreateControllerCallFunc(const iocshArgBuf *args)
{
  MMC100CreateController(args[0].sval, args[1].sval, args[2].ival, args[3].ival, args[4].ival);
}

//////////MMC100EncoderSetup
/** Creates a new MMC100Controller object.
  * Configuration command, called directly or from iocsh
  * \param[in] portName          The port name
  * \param[in] axis              The axis to configure
  * \param[in] analog            Digital (0) or analog (1) encoder
  * \param[in] resolution        Encoder resolution (0.001 ~ 999.999 um/count or mdeg/count) (double)
  * \param[in] polarity          Normal (0) or reverse operation (1) (change if encoder pos seems wrong)
  * \param[in] deadband_counts   Continuous oscillation (0) or encoder counts (>=1) (int)
  * \param[in] deadband_timeout  Time to move into the deadband area (0.0 = infinite) (double)
  * \param[in] feedback          Feedback option [0, 1, 2, 3] (int)
  */
extern "C" int 
MMC100EncoderSetup(const char *portName, int axis_num, int analog, double resolution,
                    int polarity, int deadband_counts, double deadband_timeout, int feedback)
{
  MMC100Controller* controller=findByPortName(portName);
  if (!controller)
    return asynError;
  
  MMC100Axis* axis=controller->getAxis(axis_num);
  if (!axis) {
    fprintf(stderr, "%s:%s: Invalid axis number %d (numAxes=%d)\n",
            driverName, portName, axis_num, controller->getAxisCount());
    return asynError;
  }

  axis->setupEncoder(analog, resolution, polarity, deadband_counts, deadband_timeout, feedback);
  return(asynSuccess);
}

/** Code for iocsh registration */
static const iocshArg MMC100EncoderSetupArg0 = {"Port name", iocshArgString};
static const iocshArg MMC100EncoderSetupArg1 = {"axis", iocshArgInt};
static const iocshArg MMC100EncoderSetupArg2 = {"analog", iocshArgInt};
static const iocshArg MMC100EncoderSetupArg3 = {"resolution", iocshArgDouble};
static const iocshArg MMC100EncoderSetupArg4 = {"polarity", iocshArgInt};
static const iocshArg MMC100EncoderSetupArg5 = {"deadband_counts", iocshArgInt};
static const iocshArg MMC100EncoderSetupArg6 = {"deadband_timeout", iocshArgDouble};
static const iocshArg MMC100EncoderSetupArg7 = {"feedback", iocshArgInt};
static const iocshArg * const MMC100EncoderSetupArgs[] = {&MMC100EncoderSetupArg0,
                                                          &MMC100EncoderSetupArg1,
                                                          &MMC100EncoderSetupArg2,
                                                          &MMC100EncoderSetupArg3,
                                                          &MMC100EncoderSetupArg4,
                                                          &MMC100EncoderSetupArg5,
                                                          &MMC100EncoderSetupArg6,
                                                          &MMC100EncoderSetupArg7
                                                            };
static const iocshFuncDef MMC100EncoderSetupDef = {"MMC100EncoderSetup", 8, MMC100EncoderSetupArgs};
static void MMC100EncoderSetupCallFunc(const iocshArgBuf *args)
{
  MMC100EncoderSetup(args[0].sval, args[1].ival, args[2].ival, args[3].dval, args[4].ival,
                     args[5].ival, args[6].dval, args[7].ival);
}

//////////MMC100LimitSetup
/** Creates a new MMC100Controller object.
  * Configuration command, called directly or from iocsh
  * \param[in] portName          The port name
  * \param[in] axis              The axis to configure
  * \param[in] enable            Disable (0) or enable (1) limit switches
  * \param[in] active_level      Active low (0) or active high (1)
  */
extern "C" int 
MMC100LimitSetup(const char *portName, int axis_num, int enable, int active_level)
{
  MMC100Controller* controller=findByPortName(portName);
  if (!controller)
    return asynError;
 
  MMC100Axis* axis=controller->getAxis(axis_num);
  if (!axis) {
    fprintf(stderr, "%s:%s: Invalid axis number %d (numAxes=%d)\n",
            driverName, portName, axis_num, controller->getAxisCount());
    return asynError;
  }

  axis->setupLimits(enable, active_level);
  return(asynSuccess);
}

/** Code for iocsh registration */
static const iocshArg MMC100LimitSetupArg0 = {"Port name", iocshArgString};
static const iocshArg MMC100LimitSetupArg1 = {"axis", iocshArgInt};
static const iocshArg MMC100LimitSetupArg2 = {"enable", iocshArgInt};
static const iocshArg MMC100LimitSetupArg3 = {"active_level", iocshArgInt};
static const iocshArg * const MMC100LimitSetupArgs[] = {&MMC100LimitSetupArg0,
                                                        &MMC100LimitSetupArg1,
                                                        &MMC100LimitSetupArg2,
                                                        &MMC100LimitSetupArg3
                                                            };
static const iocshFuncDef MMC100LimitSetupDef = {"MMC100LimitSetup", 4, MMC100LimitSetupArgs};
static void MMC100LimitSetupCallFunc(const iocshArgBuf *args)
{
  MMC100LimitSetup(args[0].sval, args[1].ival, args[2].ival, args[3].ival);
}


/***********************************************************************/
static void MMC100MotorRegister(void)
{
  iocshRegister(&MMC100CreateControllerDef, MMC100CreateControllerCallFunc);
  iocshRegister(&MMC100EncoderSetupDef, MMC100EncoderSetupCallFunc);
  iocshRegister(&MMC100LimitSetupDef, MMC100LimitSetupCallFunc);
}

extern "C" {
epicsExportRegistrar(MMC100MotorRegister);
}

/*
//TODO: remove
#include <asynPortDriver.h>
asynStatus asynPortDriver::clearUInt32DigitalInterrupt(int, unsigned int) { return asynSuccess; }
asynStatus asynPortDriver::setUInt32DigitalInterrupt(int, unsigned int, interruptReason) { return asynSuccess; }
asynStatus asynPortDriver::setUInt32DigitalInterrupt(int, int, unsigned int, interruptReason) { return asynSuccess; }
asynStatus asynPortDriver::clearUInt32DigitalInterrupt(int, int, unsigned int) { return asynSuccess; }
asynStatus asynPortDriver::getUInt32DigitalInterrupt(int, int, unsigned int*, interruptReason) { return asynSuccess; }
asynStatus asynPortDriver::getUInt32DigitalInterrupt(int, unsigned int*, interruptReason) { return asynSuccess; }
*/
