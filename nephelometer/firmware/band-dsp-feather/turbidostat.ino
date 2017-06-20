#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "turbidostat.h"

Turbido::Turbido(Supervisor &s, int pumpno):
  _s(s),
  _mUpper(0x7fffffff),
  _mLower(0),
  _pumpno(pumpno),
  _startSec(0),
  _startPumpMsec(0)
{
  Serial.println("# Turbido controller initialized");
}

int Turbido::begin(void)
{
  _startSec = rtcSeconds();
  _startPumpMsec = pump().totalOnMsec();

  setPumpOff();

  Serial.println("T\ttime.s\tneph\tgain\tpumpon\tpumptime.s");

  return 0;
}

int Turbido::loop(void)
{
  delayOneSecond();

  long sec = rtcSeconds();
  long m = measure();

  if (pump().isPumping() && m < mLower()) {
    setPumpOff();
  } else if ((!pump().isPumping()) && m > mUpper()) {
    setPumpOn();
  }

  long ptime = pump().totalOnMsec();

  snprintf(Supervisor::outbuf, Supervisor::outbufLen, 
           "T\t%lu\t%ld.%03ld\t%ld\t%d\t%ld.%03ld\r\n", 
           sec - _startSec, m/1000, m%1000, _s.nephelometer().pgaScale(), pump().isPumping(),
           ptime / ((long) 1000), ptime % ((long) 1000));
  Serial.write(Supervisor::outbuf);

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

long Turbido::measure(void) { return _s.nephelometer().measure(); }

const Pump &Turbido::pump(void) { return _s.pump(_pumpno); }

void Turbido::setPumpOn(void)  { _s.pump(_pumpno).setPumping(1); }

void Turbido::setPumpOff(void) { _s.pump(_pumpno).setPumping(0); }

void Turbido::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Pump on @ %ld.%03ld\r\n# Pump off @ %ld.%03ld\r\n# Pump %c\r\n", 
           _mUpper/1000, _mUpper%1000, _mLower/1000, _mLower%1000, Supervisor::pumpnoToChar(_pumpno));
}

void Turbido::manualSetParams(void)
{
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));

  manualReadParam("pump on (high) measurement", _mUpper);
  manualReadParam("pump off (low) measurement", _mLower);
  manualReadPump("pump number                ", _pumpno);
  
  serialWriteParams();
}

