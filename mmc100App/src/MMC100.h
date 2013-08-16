/*
 * Micos MMC-100 EPICS Driver
 * Ken Lauer (klauer@bnl.gov)
 * Brookhaven National Laboratory
 * */

#ifndef _MMC100_H
#define _MMC100_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cmath>

#include <iocsh.h>
#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsExport.h>

#include <asynOctetSyncIO.h>
#include "asynMotorController.h"
#include "asynMotorAxis.h"

#define MMC100_TIMEOUT              0.5
#define MMC100_STRING_SIZE          80
#define MMC100_UNIT_SCALE           1.0e6

#define MMC100_FLAG_ERRORED         0x080
#define MMC100_FLAG_ACCELERATING    0x040
#define MMC100_FLAG_CONST_VELOCITY  0x020
#define MMC100_FLAG_DECELERATING    0x010
#define MMC100_FLAG_STOPPED         0x008
#define MMC100_FLAG_PROGRAM_RUNNING 0x004
#define MMC100_FLAG_POS_SWITCH      0x002
#define MMC100_FLAG_NEG_SWITCH      0x001

static const char* driverName = "MMC100";
class MMC100Controller;

class MMC100Axis : public asynMotorAxis
{
public:
  /* These are the methods we override from the base class */
  MMC100Axis(MMC100Controller *pC, int axis);
  asynStatus move(double position, int relative, double min_velocity, double max_velocity, double acceleration);
  asynStatus moveVelocity(double min_velocity, double max_velocity, double acceleration);
  asynStatus home(double min_velocity, double max_velocity, double acceleration, int forwards);
  asynStatus stop(double acceleration);
  asynStatus poll(bool *moving);
  //asynStatus setPosition(double position);

  /* And these are specific to this class: */
  asynStatus setServo(bool enabled);
  asynStatus queryPosition();
  asynStatus queryStatus(bool* moving=NULL);

  inline bool isFlagSet(unsigned int flag) { return (flags_ & flag) == flag; }
  inline void setFlag(unsigned int flag)   { flags_ |= flag; }
  inline void clearFlag(unsigned int flag) { flags_ &= ~flag; }
  inline void setFlag(unsigned int flag, bool set) {
    if (set)
      flags_ |= flag;
    else
      flags_ &= ~flag;
  }

  asynStatus setupEncoder(int analog, double resolution, int polarity, int deadband_counts, double deadband_timeout, int feedback);
  asynStatus setupLimits(int enabled, int active_level);

protected:
  int check_error();

private:
  friend class MMC100Controller;
  MMC100Controller *pc_;      /**< Pointer to the asynMotorController to which this axis belongs.
                              *   Abbreviated because it is used very frequently */
  double encoderPos_;   /**< Cached copy of the encoder position */
  double theoryPos_;   /**< Cached copy of the theoretical position */
  unsigned int flags_;       /**< Cached copy of the current flags */

  bool homing_;
  bool errored_;
  int axisNum_;         // according to asyn
  int id_;              // axis num according to controller
  int status_failed_;   // number of consecutive times status queries have failed

};

// if status queries fail (n) times in a row, restart the ioc
#define MMC100_STATUS_FAILED_THRESHOLD 100

/* NOTE on unitScale_:
 * Trying to use 1.0 for the encoder/position resolution does not work,
 * regardless of if your encoder gives floating point position readout.
 *
 * It will result in truncation of the values in EPICS, such that you'd
 * only see 0.5, if in fact the motor was at 0.5345423. As such, the position
 * is kept properly internally, but it is scaled when being passed back
 * and forth from EPICS.
 */

class MMC100Controller : public asynMotorController {
public:
  MMC100Controller(const char *portName, const char *MMC100PortName, int numAxes, double movingPollPeriod, double idlePollPeriod);
  MMC100Axis* getAxis(int axisNo) {
    return (MMC100Axis*)asynMotorController::getAxis(axisNo);
  }

  /* These are the methods that are new to this class */
  asynStatus write(const char* fmt, ...);
  asynStatus write(const char* fmt, va_list);
  asynStatus writeRead(char* input, size_t* nread, const char* fmt, ...);
  asynStatus writeRead(char* input, size_t* nread, const char* fmt, va_list);
  int getAxisCount() { return numAxes_; }

protected:
/*  int MMC100Jerk_;          // Jerk time parameter index
#define FIRST_MMC100_PARAM MMC100Jerk_
  int MMC100ReadBinaryIO_;  //  Read binary I/O parameter index
#define LAST_MMC100_PARAM MMC100Jerk_
#define NUM_MMC100_PARAMS (&LAST_MMC100_PARAM - &FIRST_MMC100_PARAM + 1)
*/

#define NUM_MMC100_PARAMS 0
  double unitScale_; // see note above
  double timeout_;

private:
  friend class MMC100Axis;
  asynUser *pasynUser_;
  char outString_[MMC100_STRING_SIZE];
  bool homing_axis_;

  /*
  Write and read a single int value (assumes used only on one axis)
  */
  asynStatus writeReadInt(int& value, const char* fmt, ...);

};

/* Use the following structure and functions to manage multiple instances
 * of the driver */
typedef struct MMC100Node {
    ELLNODE node;
    const char *portName;
    MMC100Controller *pController;
} MMC100Node;

bool addToList(const char *portName, MMC100Controller *drv);
MMC100Controller* findByPortName(const char *portName);

#endif
