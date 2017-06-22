#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "turbidomix.h"
#include "turbidoschedule.h"

TurbidoGradient::TurbidoGradient(Supervisor &s):
  TurbidoMixBase(s),
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
  buf[0] = 'G';
  TurbidoMixBase::formatHeader(buf + 1, buflen - 1);
  strncpy(buf + strlen(buf), "\tpump1pct", buflen - strlen(buf));
}

void TurbidoGradient::formatLine(char *buf, unsigned int buflen, long m)
{
  buf[0] = 'G';
  TurbidoMixBase::formatLine(buf + 1, buflen - 1, m);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "\t%2d", pump1Percent());
}

void TurbidoGradient::formatParams(char *buf, unsigned int buflen)
{
  TurbidoMixBase::formatParams(buf, buflen);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "# Pump #1 start %d%%\r\n# Pump #1 step %d%%\r\n# Number of steps %ld\r\n# Step time (seconds) %ld\r\n", 
           _pump1StartPct, _pump1StepPct, _nSteps, _stepTime);
}

void TurbidoGradient::manualReadParams(void)
{
  TurbidoMixBase::manualReadParams();
  manualReadPercent("pump #1 percentage, start", _pump1StartPct);
  manualReadPercent("pump #1 percentage, step", _pump1StepPct);
  manualReadLong("number of steps", _nSteps);
  manualReadLong("time per step (seconds)", _stepTime);
}

TurbidoCycle::TurbidoCycle(Supervisor &s):
  TurbidoMixBase(s),
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
  buf[0] = 'C';
  TurbidoMixBase::formatHeader(buf + 1, buflen - 1);
  strncpy(buf + strlen(buf), "\tpump1pct", buflen - strlen(buf));
}

void TurbidoCycle::formatLine(char *buf, unsigned int buflen, long m)
{
  buf[0] = 'C';
  TurbidoMixBase::formatLine(buf + 1, buflen - 1, m);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "\t%2d", pump1Percent());
}

void TurbidoCycle::formatParams(char *buf, unsigned int buflen)
{
  TurbidoMixBase::formatParams(buf, buflen);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "# Pump #1 phase 1 %d%%\r\n# Pump #1 phase 2 %d%%\r\n# Phase 1 time (seconds) %ld\r\n# Phase 2 time (seconds) %ld\r\n", 
           _pump1FirstPct, _pump1SecondPct, _firstTime, _secondTime);
}

void TurbidoCycle::manualReadParams(void)
{
  TurbidoMixBase::manualReadParams();
  manualReadPercent("pump #1 percentage, phase 1", _pump1FirstPct);
  manualReadPercent("pump #1 percentage, phase 2", _pump1SecondPct);
  manualReadLong("time in phase 1 (seconds)", _firstTime);
  manualReadLong("time in phase 2 (seconds)", _secondTime);
}


