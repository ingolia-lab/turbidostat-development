#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "turbidomix.h"

TurbidoMix::TurbidoMix(Supervisor &s):
  _s(s),
  _mUpper(0x7fffffff),
  _mLower(0),
  _pump1(0),
  _pump2(1),
  _pump1Pct(50),
  _startSec(0),
  _startPump1Msec(0),
  _startPump2Msec(0),
  _cycleCount(0)
{
  Serial.println("# TurbidoMix controller initialized");
}

int TurbidoMix::begin(void)
{
  _startSec = rtcSeconds();
  _startPump1Msec = pump1().totalOnMsec();
  _startPump2Msec = pump2().totalOnMsec();

  setPumpNone();

  _cycleCount = 0;

  Serial.println("U\ttime.s\tneph\tgain\tpump1on\tpump2on\tpump1.s\tpump2.s");

  return 0;
}

int TurbidoMix::loop(void)
{
  delayOneSecond();

  long sec = rtcSeconds();
  long m = measure();

  if (eitherPumping()) {
    if (m < mLower()) {
      setPumpNone();
    } else {
      setPumpOnCycle();
    }
  } else if (m > mUpper()) {
    setPumpOnCycle();    
  }

  long time1 = pump1().totalOnMsec(), time2 = pump2().totalOnMsec();

  snprintf(Supervisor::outbuf, Supervisor::outbufLen, 
           "U\t%lu\t%ld\t%ld\t%d\t%d\t%ld.%03ld\t%ld.%03ld\r\n", 
           sec - _startSec, m, _s.nephelometer().pgaScale(), 
           pump1().isPumping(), pump2().isPumping(),
           time1 / ((long) 1000), time1 % ((long) 1000),
           time2 / ((long) 1000), time2 % ((long) 1000));
  Serial.write(Supervisor::outbuf);

  int ch;
  while ((ch = Serial.read()) >= 0) {
    if (ch == 'q') {
      setPumpNone();
      return 1;
      while (Serial.read() >= 0) {
        /* DISCARD */ 
      }
    } 
  }

  return 0;
}

long TurbidoMix::measure(void) { return _s.nephelometer().measure(); }

const Pump &TurbidoMix::pump1(void) { return _s.pump(_pump1); }
const Pump &TurbidoMix::pump2(void) { return _s.pump(_pump2); }

void TurbidoMix::setPump1(void)     { _s.pump(_pump1).setPumping(1); _s.pump(_pump2).setPumping(0); }
void TurbidoMix::setPump2(void)     { _s.pump(_pump1).setPumping(0); _s.pump(_pump2).setPumping(1); }
void TurbidoMix::setPumpNone(void)  { _s.pump(_pump1).setPumping(0); _s.pump(_pump2).setPumping(0); }

void TurbidoMix::setPumpOnCycle(void)
{
  unsigned long count = pumpCountIncr();

  if (schedulePercent(_pump1Pct, (uint8_t) (count % 100))) {  
    setPump1();
  } else {
    setPump2();
  }
}

void TurbidoMix::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Pump on @ %ld\r\n# Pump off @ %ld\r\n# Pump #1 %d%% share\r\n", 
           _mUpper, _mLower, _pump1Pct);
}

void TurbidoMix::manualSetParams(void)
{
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));

  manualReadParam("pump on (high) measurement", _mUpper);
  manualReadParam("pump off (low) measurement", _mLower);
  manualReadPump("pump #1                    ", _pump1);
  manualReadPump("pump #2                    ", _pump2);
  manualReadPercent("pump #1 percentage      ", _pump1Pct);
  
  serialWriteParams();
}

