/*
 * Micos MMC-100 EPICS Driver
 * Ken Lauer (klauer@bnl.gov)
 * Brookhaven National Laboratory
 * */

#include "MMC100.h"

/** Creates a new MMC100Axis object.
  * \param[in] controller         The MMC100 controller
  * \param[in] axis_num           The axis number (1-based)
  */
MMC100Axis::MMC100Axis(MMC100Controller *controller, int axis_num)
  :  asynMotorAxis((asynMotorController*)controller, axis_num)
{
    id_ = axis_num+1;
    pc_ = controller;
    homing_ = false;
    encoderPos_ = 0.0;
    theoryPos_ = 0.0;
    status_failed_ = 0;
}


asynStatus MMC100Axis::poll(bool *moving) {
  if (pc_->homing_axis_) // cannot send any commands while homing
    return asynSuccess;

  pc_->lock();
  queryStatus(moving);
  queryPosition();
  pc_->unlock();

  callParamCallbacks();

  return asynSuccess;
}

/** Creates a new MMC100Controller object.
  * \param[in] portName          The name of the asyn port that will be created for this driver
  * \param[in] MMC100PortName     The name of the drvAsynIPPPort that was created previously to connect to the MMC100 controller
  * \param[in] numAxes           The number of axes that this controller supports
  * \param[in] movingPollPeriod  The time between polls when any axis is moving
  * \param[in] idlePollPeriod    The time between polls when no axis is moving
  */
MMC100Controller::MMC100Controller(const char *portName, const char *MMC100PortName, int numAxes,
                             double movingPollPeriod, double idlePollPeriod)
  :  asynMotorController(portName, numAxes, NUM_MMC100_PARAMS,
                         asynInt32Mask | asynFloat64Mask | asynUInt32DigitalMask,
                         asynInt32Mask | asynFloat64Mask | asynUInt32DigitalMask,
                         ASYN_CANBLOCK | ASYN_MULTIDEVICE,
                         1, // autoconnect
                         0, 0)  // Default priority and stack size
{
  int axis;
  asynStatus status;
  MMC100Axis *pAxis;
  static const char *functionName = "MMC100Controller::MMC100Controller";

  unitScale_ = MMC100_UNIT_SCALE;
  if (!addToList(portName, this)) {
    printf("%s:%s: Init failed", driverName, portName);
    return;
  }

  printf("Idle: %f moving: %f\n", idlePollPeriod_, movingPollPeriod_);
  idlePollPeriod_ = idlePollPeriod;
  movingPollPeriod_ = movingPollPeriod;
  timeout_ = MMC100_TIMEOUT;
  homing_axis_ = false;

  /* Connect to MMC100 controller */
  status = pasynOctetSyncIO->connect(MMC100PortName, 0, &pasynUser_, NULL);
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s:%s: cannot connect to MMC100 controller\n",
      driverName, functionName);
  }

  status = pasynOctetSyncIO->setInputEos(pasynUser_, "\n\r", 2);
  if (status) {
    asynPrint(pasynUser_, ASYN_TRACE_ERROR|ASYN_TRACE_FLOW,
      "%s: Unable to set input EOS on %s: %s\n",
      functionName, MMC100PortName, pasynUser_->errorMessage);
  }

  status = pasynOctetSyncIO->setOutputEos(pasynUser_, "\n\r", 2);
  if (status) {
    asynPrint(pasynUser_, ASYN_TRACE_ERROR|ASYN_TRACE_FLOW,
      "%s: Unable to set output EOS on %s: %s\n",
      functionName, MMC100PortName, pasynUser_->errorMessage);
  }

  // Create the axis objects
  for (axis=0; axis<numAxes; axis++) {
    pAxis = new MMC100Axis(this, axis);
    pAxis->check_error();
  }
  startPoller(movingPollPeriod/1000., idlePollPeriod/1000., 2);
}

asynStatus MMC100Controller::write(const char *fmt, va_list argptr) {
  size_t nwrite;
  asynStatus status;
  const char *functionName="write";
  const int buf_size = MMC100_STRING_SIZE;
  char buf[buf_size];

  vsnprintf(buf, buf_size, fmt, argptr);

  lock();

#if DEBUG
  fprintf(stderr, "%s:%s: %s\n", driverName, functionName, buf);
#endif

  asynPrint(pasynUser_, ASYN_TRACE_FLOW,
    "%s:%s: %s\n",
    driverName, functionName, buf);

  status = pasynOctetSyncIO->write(pasynUser_,
                                   buf, strlen(buf),
                                   timeout_, &nwrite);
  unlock();

  return status;
}

asynStatus MMC100Controller::write(const char *fmt, ...) {
  va_list argptr;
  va_start(argptr,fmt);
  asynStatus ret=write(fmt, argptr);
  va_end(argptr);
  return ret;
}

asynStatus MMC100Controller::writeReadInt(int& value, const char* fmt, ...) {
  static const char* functionName = "MMC100Controller::writeReadInt";
  size_t read;
  va_list argptr;
  char input[MMC100_STRING_SIZE];

  va_start(argptr,fmt);
  asynStatus status = writeRead(input, &read, fmt, argptr);
  va_end(argptr);

  if (status != asynSuccess || input[0] != '#') {
    if (read == 0) {
      asynPrint(pasynUser_, ASYN_TRACE_ERROR,
          "%s: writeRead failed: %s (ret=%d) no data received\n",
          functionName, outString_, status);
    } else {
      asynPrint(pasynUser_, ASYN_TRACE_ERROR,
          "%s: writeRead failed: %s (ret=%d) received: %s\n",
          functionName, outString_, status, input);
    }
    return asynError;
  } else {
    value = atoi(&input[1]);
  }
  return status;
}

asynStatus MMC100Controller::writeRead(char *input, size_t* nread, const char *fmt, va_list argptr) {

  size_t nwrite;
  asynStatus status;
  int eomReason;
  const char *functionName="writeRead";
  const int buf_size = MMC100_STRING_SIZE;
  char buf[buf_size];

  vsnprintf(buf, buf_size, fmt, argptr);

  strncpy(outString_, buf, buf_size);
  lock();
#if DEBUG
  fprintf(stderr, "%s:%s: write: %s\n",
    driverName, functionName, buf);
#endif

  asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
    "%s:%s: Write: %s\n",
    driverName, functionName, buf);

  status = pasynOctetSyncIO->writeRead(pasynUser_,
                                       buf, strlen(buf),
                                       input, MMC100_STRING_SIZE,
                                       timeout_, &nwrite, nread, &eomReason);

  if (*nread == 0) {
    input[*nread] = 0;
  }

#if DEBUG
  fprintf(stderr, "%s:%s: Read (%db): %s\n",
    driverName, functionName, *nread, input);
#endif
  asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
    "%s:%s: Read (%db): %s\n",
    driverName, functionName, (int)(*nread), input);

  unlock();

  return status;
}

asynStatus MMC100Controller::writeRead(char *input, size_t* nread, const char *fmt, ...) {
  va_list argptr;
  va_start(argptr,fmt);
  asynStatus ret=writeRead(input, nread, fmt, argptr);
  va_end(argptr);
  return ret;
}

asynStatus MMC100Axis::setupEncoder(int analog, double resolution, int polarity, int deadband_counts, double deadband_timeout, int feedback) {
  static const char* functionName = "setupEncoder";
  if (analog < 0 || analog > 1) {
    asynPrint(pc_->pasynUser_, ASYN_TRACE_ERROR|ASYN_TRACE_FLOW,
      "%s: Invalid 'analog' value %d, should be in [0, 1]\n",
      functionName, analog);
  } else {
    pc_->write("%dEAD%d", id_, analog);
    check_error();
  }

  if (resolution < 0.001 || resolution > 999.999) {
    asynPrint(pc_->pasynUser_, ASYN_TRACE_ERROR|ASYN_TRACE_FLOW,
      "%s: Invalid 'resolution' value %g, should be in [0.001, 999.999]\n",
      functionName, resolution);
  } else {
    pc_->write("%dENC%3.3f", id_, resolution);
    check_error();
  }

  if (polarity < 0 || polarity > 1) {
    asynPrint(pc_->pasynUser_, ASYN_TRACE_ERROR|ASYN_TRACE_FLOW,
      "%s: Invalid 'polarity' value %d, should be in [0, 1]\n",
      functionName, polarity);
  } else {
    pc_->write("%dEPL%d", id_, polarity);
    check_error();
  }

  if (deadband_counts < 0) {
    asynPrint(pc_->pasynUser_, ASYN_TRACE_ERROR|ASYN_TRACE_FLOW,
      "%s: Invalid 'deadband_counts' value %d, should be >= 0\n",
      functionName, deadband_counts);
  } else if (deadband_timeout < 0.0) {
    asynPrint(pc_->pasynUser_, ASYN_TRACE_ERROR|ASYN_TRACE_FLOW,
      "%s: Invalid 'deadband_timeout' value %g, should be >= 0.0\n",
      functionName, deadband_timeout);
  } else {
    pc_->write("%dDBD%d,%3.3f", id_, deadband_counts, deadband_timeout);
    check_error();
  }

  if (feedback < 0 || feedback > 3) {
    asynPrint(pc_->pasynUser_, ASYN_TRACE_ERROR|ASYN_TRACE_FLOW,
      "%s: Invalid 'feedback' value %d, should be 0 <= feedback <= 3\n",
      functionName, feedback);
  } else {
    pc_->write("%dFBK%d", id_, feedback);
    check_error();
  }
  return asynSuccess;
}

asynStatus MMC100Axis::setupLimits(int enabled, int active_level) {
  static const char* functionName = "setupLimits";
  asynPrint(pc_->pasynUser_, ASYN_TRACE_ERROR|ASYN_TRACE_FLOW,
    "%s:%s\n",
    driverName, functionName);
  if (enabled < 0 || enabled > 1) {
    asynPrint(pc_->pasynUser_, ASYN_TRACE_ERROR|ASYN_TRACE_FLOW,
      "%s: Invalid 'enabled' value %d, should be in [0, 1]\n",
      functionName, enabled);
  } else {
    pc_->write("%dLCG%d", id_, enabled);
    check_error();
  }

  if (active_level < 0 || active_level > 1) {
    asynPrint(pc_->pasynUser_, ASYN_TRACE_ERROR|ASYN_TRACE_FLOW,
      "%s: Invalid 'active_level' value %d, should be in [0, 1]\n",
      functionName, active_level);
  } else {
    pc_->write("%dLPL%d", id_, active_level);
    check_error();
  }
  return asynSuccess;
}

#ifndef strnchr

char* strnchr(const char* str, size_t len, char c) {
  if (!str)
    return NULL;

  while (len > 0 && *str != '\0') {
    if (*str == c) {
      return (char*)str;
    }
    str++;
    len--;
  }
  return NULL;
}

#endif

asynStatus MMC100Axis::queryPosition() {
  if (pc_->homing_axis_)
    return asynError;

  size_t num_read=0;
  char input[MMC100_STRING_SIZE];
  //static const char* functionName = "MMC100Controller::queryPosition";
  asynStatus ret = pc_->writeRead(input, &num_read, "%dPOS?", id_);
  if (ret != asynSuccess || num_read < 17) // 17 == len("#0.000000,0.000000")
    return asynError;

  // Format: #theory,encoder
  // So, find the comma, replace it with a null terminator, and
  // use atof on each of them.
  char *theory_pos=&input[1];
  char *encoder_pos=strnchr(input, MMC100_STRING_SIZE, ',');
  if (!encoder_pos) {
    return asynError;
  }

  encoder_pos[0] = '\0';
  encoder_pos++;

  encoderPos_=atof(encoder_pos);
  theoryPos_=atof(theory_pos);
  setDoubleParam(pc_->motorEncoderPosition_, encoderPos_ * pc_->unitScale_);
  setDoubleParam(pc_->motorPosition_, encoderPos_ * pc_->unitScale_);
  //setDoubleParam(pc_->motorPosition_, theoryPos_ * pc_->unitScale_);

#if DEBUG
  fprintf(stderr, "encoder: %g theory: %g\n",
              encoderPos_, theoryPos_);
#endif
  return asynSuccess;
}

asynStatus MMC100Axis::queryStatus(bool* moving) {
  if (pc_->homing_axis_ && !homing_)
    return asynError;
  int status=0;
  //static const char* functionName = "MMC100Controller::queryStatus";
  asynStatus ret = pc_->writeReadInt(status, "%dSTA?", id_);

  if (ret == asynSuccess) {
    errored_ = ((status & 0x080) == 0x080);

    bool stopped = ((status & 0x008) == 0x008);

#if DEBUG
    bool accelerating = ((status & 0x040) == 0x040);
    bool const_velocity = ((status & 0x020) == 0x020);
    bool decelerating = ((status & 0x010) == 0x010);

    bool program_running = ((status & 0x004) == 0x004);
    bool pos_switch = ((status & 0x002) == 0x002);
    bool neg_switch = ((status & 0x001) == 0x001);

    fprintf(stderr, "errored: %d accelerating: %d const vel: %d decelerating: %d stopped: %d program running: %d pos switch: %d neg sw: %d\n",
                errored_, accelerating, const_velocity, decelerating, stopped, program_running, pos_switch, neg_switch);
#endif

    flags_ = status;
    if (moving) {
      *moving = !stopped;
    }

    if (stopped && homing_) {
      homing_ = false;
      pc_->homing_axis_ = false;
    }

    setIntegerParam(pc_->motorStatusMoving_, !stopped);
    setIntegerParam(pc_->motorStatusDone_, stopped);
    status_failed_ = 0;
  } else {
    status_failed_++;
    if (status_failed_ > MMC100_STATUS_FAILED_THRESHOLD) {
      exit(1);
    }
  }

  return ret;
}

int
MMC100Axis::check_error() {
  if (pc_->homing_axis_ && !homing_)
    return asynError;

  size_t read;
  int error=0;
  static const char* functionName = "MMC100Controller::check_error";
  char input[MMC100_STRING_SIZE];
  queryStatus();
  if (errored_) {
    asynStatus status = pc_->writeRead(input, &read, "%dERR?", id_);
    const char *err;
    if (status == asynSuccess && read > 0) {
      err = strnchr(input, MMC100_STRING_SIZE, '-');
      if (err) {
        asynPrint(pc_->pasynUser_, ASYN_TRACE_ERROR|ASYN_TRACE_FLOW,
          "%s:%s: axis: %d error: %s\n",
          driverName, functionName, id_, err);
        error = 1;
      }
    }
    if (!error) {
      asynPrint(pc_->pasynUser_, 0x255, //ASYN_TRACE_FLOW,
        "%s:%s: axis: %d success (or no response)\n",
        driverName, functionName, id_);
    }
  }
  return error;
}

asynStatus MMC100Axis::stop(double acceleration)
{
  asynStatus status=pc_->write("%dSTP", id_);
  check_error();
  return status;
}

asynStatus MMC100Axis::moveVelocity(double min_velocity, double max_velocity, double acceleration) {
  if (pc_->homing_axis_)
    return asynError;

  asynStatus ret=pc_->write("%dJOG50.0");
  check_error();
  return ret;
}

asynStatus MMC100Axis::home(double min_velocity, double max_velocity, double acceleration, int forwards) {
  static const char* functionName = "MMC100Axis::home";
  asynStatus ret;
  if (pc_->homing_axis_)
    return asynError;

  pc_->lock();

  pc_->homing_axis_ = true;
  homing_ = true;
//#if DEBUG
  fprintf(stderr, "%s:%d: homing started\n", functionName, id_);
//#endif

  ret = pc_->write("%dHCG%d", id_, (forwards != 0 ? 1 : 0));
  check_error();

  if (ret != asynSuccess) {
    fprintf(stderr, "%s:%d: homing failed to set direction\n", functionName, id_);
    pc_->unlock();
    return ret;
  }

  ret = pc_->write("%dHOM", id_);

  if (ret != asynSuccess) {
    fprintf(stderr, "%s:%d: homing failed\n", functionName, id_);
    pc_->unlock();
    return ret;
  }

  pc_->timeout_ = 10.0 * MMC100_TIMEOUT;
  check_error();
  pc_->timeout_ = MMC100_TIMEOUT;

//#if DEBUG
  fprintf(stderr, "%s:%d: homing done\n", functionName, id_);
//#endif

  pc_->unlock();
  return asynSuccess;
}

asynStatus MMC100Axis::move(double position, int relative, double min_velocity, double max_velocity, double acceleration)
{
  if (pc_->homing_axis_)
    return asynError;

  static const char* functionName = "MMC100Axis::move";
  asynStatus ret;
  double velocity;

  position /= pc_->unitScale_;
  min_velocity /= pc_->unitScale_;
  max_velocity /= pc_->unitScale_;

  pc_->getDoubleParam(axisNo_, pc_->motorVelocity_, &velocity);
  velocity /= pc_->unitScale_;

  if (velocity > max_velocity)
    velocity = max_velocity;
  else if (velocity < min_velocity)
    velocity = min_velocity;

  asynPrint(pc_->pasynUser_, ASYN_TRACE_FLOW,
    "%s:%s: axis %d: move to %g (relative=%d) at velocity %g\n",
    driverName, functionName, id_, position, relative, velocity);

  ret = pc_->write("%dVEL%3.3f", id_, velocity);
  check_error();
  if (ret != asynSuccess) {
    return ret;
  }

  if (relative != 0) {
    ret=pc_->write("%dMVR%3.6f", id_, position);
  } else {
    // absolute
    ret=pc_->write("%dMVA%3.6f", id_, position);
  }

  printf("Move %d to %3.6f\n", id_, position);
  check_error();
  return ret;
}

