#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "turbidobase.h"

#define LINEBUF_LEN 256
const unsigned int TurbidoBase::linebufLen = LINEBUF_LEN;
char TurbidoBase::linebuf[LINEBUF_LEN];

TurbidoBase::TurbidoBase(Supervisor &s):
  _s(s),
  _mUpper(Nephel::maxMeasure + 1),
  _mLower(0),
  _startSec(0)
{

}

int TurbidoBase::begin(void)
{
  _startSec = rtcSeconds();

  setPumpOff();

  formatHeader(linebuf, linebufLen);
  Serial.println(linebuf);

  for (int i = 0; i < _nMeasure; i++) {
    _measures[i] = 0;
  }
  _currMeasure = 0;

  return 0;
}

int TurbidoBase::loop(void)
{
  long m = measure();

  _currMeasure++;
  if (_currMeasure > _nMeasure) {
    _currMeasure = 0;
  }
  _measures[_currMeasure] = m;

  if (pumpMeasureOverride()) {
    // Density-dependent pumping overridden
  } else if (isLow()) {
    setPumpOff();
  } else if (isHigh()) {
    setPumpOn();
  }

  formatLine(linebuf, linebufLen, m);
  Serial.println(linebuf);

  delayOneSecond();

  int ch;
  while ((ch = Serial.read()) >= 0) {
    if (ch == 'q') {
      setPumpOff();
      return 1;
      while (Serial.read() >= 0) {
        /* DISCARD */ 
      }
    } 
  }

  return 0;
}

int TurbidoBase::isLow(void)
{
  int nLow = 0;
  for (int i = 0; i < _nMeasure; i++) {
    if (_measures[i] < mLower()) {
      nLow++;      
    }
  }
  return (nLow * 2) > _nMeasure;
}

int TurbidoBase::isHigh(void)
{
  int nHigh = 0;
  for (int i = 0; i < _nMeasure; i++) {
    if (_measures[i] > mUpper()) {
      nHigh++;      
    }
  }
  return (nHigh * 2) > _nMeasure;
}

void TurbidoBase::formatHeader(char *buf, unsigned int buflen)
{
  strncpy(buf, "\ttime.s\tneph\tgain", buflen);    
}

void TurbidoBase::formatLine(char *buf, unsigned int buflen, long m)
{
  long sec = rtcSeconds();

  snprintf(buf, buflen, 
           "\t%lu\t%ld.%03ld\t%ld", 
           sec - startSec(), m / 1000, m % 1000, s().nephelometer().pgaScale());
}

long TurbidoBase::measure(void) { return _s.nephelometer().measure(); }

void TurbidoBase::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Pump on @ %ld.%03ld\r\n# Pump off @ %ld.%03ld\r\n", 
           _mUpper/1000, _mUpper%1000, _mLower/1000, _mLower%1000);
}

void TurbidoBase::manualReadParams(void)
{
  manualReadMeasure("pump on (high) measurement", _mUpper);
  do {
    manualReadMeasure("pump off (low) measurement", _mLower);
  } while (_mLower > _mUpper);
}

