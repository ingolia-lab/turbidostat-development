#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "turbidobase.h"

#define LINEBUF_LEN 256
const unsigned int TurbidoBase::linebufLen = LINEBUF_LEN;
char TurbidoBase::linebuf[LINEBUF_LEN];

TurbidoBase::TurbidoBase(Supervisor &s):
  _s(s),
  _mUpper(0x7fffffff),
  _mLower(0),
  _pumping(0),
  _startSec(0)
{

}

int TurbidoBase::begin(void)
{
  _startSec = rtcSeconds();

  _pumping = 0;
  setPumpOff();

  formatHeader(linebuf, linebufLen);
  Serial.println(linebuf);

  return 0;
}

int TurbidoBase::loop(void)
{
  delayOneSecond();

  long m = measure();

  if (m < mLower()) {
    _pumping = 0;
    setPumpOff();
  } else if (m > mUpper() || _pumping) {
    _pumping = 1;
    setPumpOn();
  }

  formatLine(linebuf, linebufLen, m);
  Serial.println(Supervisor::outbuf);

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

long TurbidoBase::measure(void) { return _s.nephelometer().measure(); }

void TurbidoBase::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Pump on @ %ld.%03ld\r\n# Pump off @ %ld.%03ld\r\n", 
           _mUpper/1000, _mUpper%1000, _mLower/1000, _mLower%1000);
}

void TurbidoBase::manualReadParams(void)
{
  manualReadMeasure("pump on (high) measurement", _mUpper);
  manualReadMeasure("pump off (low) measurement", _mLower);
}

