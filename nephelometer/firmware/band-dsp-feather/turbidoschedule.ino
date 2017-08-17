#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "turbidomix.h"
#include "turbidoschedule.h"

TurbidoGradient::TurbidoGradient(Supervisor &s):
  TurbidoRatioBase(s),
  _pump1StartPct(50),
  _pump1StepPct(0),
  _nSteps(10),
  _stepTime(3600)
{

}

uint8_t TurbidoGradient::pump1Percent()
{
  long runningSecs = rtcSeconds() - startSec();
  long stepno = runningSecs / _stepTime;
  stepno = (stepno >= _nSteps) ? (_nSteps - 1) : stepno;
  long pctl = ((long) _pump1StartPct) + stepno * ((long) _pump1StepPct);
  return (pctl < 0) ? 0 : ((pctl > 100) ? 100 : ( (uint8_t) pctl ));
}


void TurbidoGradient::formatHeader(char *buf, unsigned int buflen)
{
  strncpy(buf, "GTR", buflen);
  TurbidoRatioBase::formatHeader(buf + strlen(buf), buflen - strlen(buf));
  strncpy(buf + strlen(buf), "\tpump1pct", buflen - strlen(buf));
}

void TurbidoGradient::formatLine(char *buf, unsigned int buflen, long m)
{
  strncpy(buf, "GTR", buflen);
  TurbidoRatioBase::formatLine(buf + strlen(buf), buflen - strlen(buf), m);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "\t%2d", pump1Percent());
}

void TurbidoGradient::formatParams(char *buf, unsigned int buflen)
{
  TurbidoRatioBase::formatParams(buf, buflen);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "# Pump #1 start %d%%\r\n# Pump #1 step %d%%\r\n# Number of steps %ld\r\n# Step time (seconds) %ld\r\n", 
           _pump1StartPct, _pump1StepPct, _nSteps, _stepTime);
}

void TurbidoGradient::manualReadParams(void)
{
  TurbidoRatioBase::manualReadParams();
  manualReadPercent("pump #1 percentage, start", _pump1StartPct);
  manualReadPercent("pump #1 percentage, step", _pump1StepPct);
  manualReadLong("number of steps", _nSteps);
  manualReadLong("time per step (seconds)", _stepTime);
}

TurbidoCycle::TurbidoCycle(Supervisor &s):
  TurbidoRatioBase(s),
  _pump1FirstPct(100),
  _pump1SecondPct(0),
  _firstTime(3600),
  _secondTime(3600)
{

}

uint8_t TurbidoCycle::pump1Percent()
{
  long runningSecs = rtcSeconds() - startSec();
  long cycleTime = _firstTime + _secondTime;

  return ((runningSecs % cycleTime) <= _firstTime) ? _pump1FirstPct : _pump1SecondPct;
}


void TurbidoCycle::formatHeader(char *buf, unsigned int buflen)
{
  strncpy(buf, "CTR", buflen);
  TurbidoRatioBase::formatHeader(buf + strlen(buf), buflen - strlen(buf));
  strncpy(buf + strlen(buf), "\tpump1pct", buflen - strlen(buf));
}

void TurbidoCycle::formatLine(char *buf, unsigned int buflen, long m)
{
  strncpy(buf, "CTR", buflen);
  TurbidoRatioBase::formatLine(buf + strlen(buf), buflen - strlen(buf), m);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "\t%2d", pump1Percent());
}

void TurbidoCycle::formatParams(char *buf, unsigned int buflen)
{
  TurbidoRatioBase::formatParams(buf, buflen);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "# Pump #1 phase 1 %d%%\r\n# Pump #1 phase 2 %d%%\r\n# Phase 1 time (seconds) %ld\r\n# Phase 2 time (seconds) %ld\r\n", 
           _pump1FirstPct, _pump1SecondPct, _firstTime, _secondTime);
}

void TurbidoCycle::manualReadParams(void)
{
  TurbidoRatioBase::manualReadParams();
  manualReadPercent("pump #1 percentage, phase 1", _pump1FirstPct);
  manualReadPercent("pump #1 percentage, phase 2", _pump1SecondPct);
  manualReadLong("time in phase 1 (seconds)", _firstTime);
  manualReadLong("time in phase 2 (seconds)", _secondTime);
}

TurbidoConcGradient::TurbidoConcGradient(Supervisor &s):
  TurbidoConcBase(s),
  _startTargetPpm1(1000000),
  _stepTargetPpm1(0),
  _nSteps(10),
  _stepTime(3600)
{

}

unsigned long TurbidoConcGradient::targetPpm1()
{
  long runningSecs = rtcSeconds() - startSec();
  long stepno = runningSecs / _stepTime;
  stepno = (stepno >= _nSteps) ? (_nSteps - 1) : stepno;
  long pctl = ((long) _startTargetPpm1) + stepno * (_stepTargetPpm1);
  if (pctl < 0) {
    return 0;
  } else if (pctl > 1000000) {
    return 1000000;
  } else {
    return (unsigned long) pctl;
  }
}


void TurbidoConcGradient::formatHeader(char *buf, unsigned int buflen)
{
  strncpy(buf, "GTC", buflen);
  TurbidoConcBase::formatHeader(buf + strlen(buf), buflen - strlen(buf));
}

void TurbidoConcGradient::formatLine(char *buf, unsigned int buflen, long m)
{
  strncpy(buf, "GTC", buflen);
  TurbidoConcBase::formatLine(buf + strlen(buf), buflen - strlen(buf), m);
}

void TurbidoConcGradient::formatParams(char *buf, unsigned int buflen)
{
  TurbidoConcBase::formatParams(buf, buflen);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "# Start fraction media #1 [ppm]: %07lu\r\n# Step fraction media #1 %07ld\r\n# Number of steps %ld\r\n# Step time (seconds) %ld\r\n", 
           _startTargetPpm1, _stepTargetPpm1, _nSteps, _stepTime);
}

void TurbidoConcGradient::manualReadParams(void)
{
  TurbidoConcBase::manualReadParams();
  manualReadULong("media #1 target, start [ppm]", _startTargetPpm1);
  manualReadLong("media #1 target, step [ppm]", _stepTargetPpm1);
  manualReadLong("number of steps", _nSteps);
  manualReadLong("time per step (seconds)", _stepTime);
}

TurbidoConcLogGradient::TurbidoConcLogGradient(Supervisor &s):
  TurbidoConcBase(s),
  _startTargetPpm1(1000000),
  _stepPct(50),
  _nSteps(10),
  _stepTime(3600)
{

}

unsigned long TurbidoConcLogGradient::targetPpm1()
{
  long runningSecs = rtcSeconds() - startSec();
  long stepno = runningSecs / _stepTime;
  stepno = (stepno >= _nSteps) ? (_nSteps - 1) : stepno;

  long pctl = (long) _startTargetPpm1;
  
  for (long i = 0; i < stepno; i++) {
    pctl = (pctl * ((long) _stepPct)) / 100;
  }
  
  if (pctl < 0) {
    return 0;
  } else if (pctl > 1000000) {
    return 1000000;
  } else {
    return (unsigned long) pctl;
  }
}


void TurbidoConcLogGradient::formatHeader(char *buf, unsigned int buflen)
{
  strncpy(buf, "GTC", buflen);
  TurbidoConcBase::formatHeader(buf + strlen(buf), buflen - strlen(buf));
}

void TurbidoConcLogGradient::formatLine(char *buf, unsigned int buflen, long m)
{
  strncpy(buf, "GTC", buflen);
  TurbidoConcBase::formatLine(buf + strlen(buf), buflen - strlen(buf), m);
}

void TurbidoConcLogGradient::formatParams(char *buf, unsigned int buflen)
{
  TurbidoConcBase::formatParams(buf, buflen);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "# Start fraction media #1 [ppm]: %07lu\r\n# Step factor %lu%%\r\n# Number of steps %ld\r\n# Step time (seconds) %ld\r\n", 
           _startTargetPpm1, _stepPct, _nSteps, _stepTime);
}

void TurbidoConcLogGradient::manualReadParams(void)
{
  TurbidoConcBase::manualReadParams();
  manualReadULong("media #1 target, start [ppm]", _startTargetPpm1);
  manualReadULong("step factor [percent]", _stepPct);
  if (_stepPct > 2000) {
    _stepPct = 2000;
  }
  manualReadLong("number of steps", _nSteps);
  manualReadLong("time per step (seconds)", _stepTime);
}

TurbidoConcCycle::TurbidoConcCycle(Supervisor &s):
  TurbidoConcBase(s),
  _firstTargetPpm1(1000000),
  _secondTargetPpm1(0),
  _firstTime(3600),
  _secondTime(3600)
{

}

unsigned long TurbidoConcCycle::targetPpm1()
{
  long runningSecs = rtcSeconds() - startSec();
  long cycleTime = _firstTime + _secondTime;

  return ((runningSecs % cycleTime) <= _firstTime) ? _firstTargetPpm1 : _secondTargetPpm1;
}


void TurbidoConcCycle::formatHeader(char *buf, unsigned int buflen)
{
  strncpy(buf, "CTC", buflen);
  TurbidoConcBase::formatHeader(buf + strlen(buf), buflen - strlen(buf));
}

void TurbidoConcCycle::formatLine(char *buf, unsigned int buflen, long m)
{
  strncpy(buf, "CTC", buflen);
  TurbidoConcBase::formatLine(buf + strlen(buf), buflen - strlen(buf), m);
}

void TurbidoConcCycle::formatParams(char *buf, unsigned int buflen)
{
  TurbidoConcBase::formatParams(buf, buflen);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "# Phase 1 target %07lu ppm media #1\r\n# Phase 2 target %07lu ppm media #1\r\n# Phase 1 time (seconds) %ld\r\n# Phase 2 time (seconds) %ld\r\n", 
           _firstTargetPpm1, _secondTargetPpm1, _firstTime, _secondTime);
}

void TurbidoConcCycle::manualReadParams(void)
{
  TurbidoConcBase::manualReadParams();
  manualReadULong("media #1 target, phase 1 [ppm]", _firstTargetPpm1);
  manualReadULong("media #1 target, phase 2 [ppm]", _secondTargetPpm1);
  manualReadLong("time in phase 1 [seconds]", _firstTime);
  manualReadLong("time in phase 2 [seconds]", _secondTime);
}
