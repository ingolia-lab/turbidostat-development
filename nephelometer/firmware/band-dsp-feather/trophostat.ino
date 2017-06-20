#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "trophostat.h"

Tropho::Tropho(Supervisor &s, int goodPumpno, int badPumpno):
  _s(s),
  _mUpper(0x7fffffff),
  _mBad(0x7fffff),
  _mGood(0),
  _mLower(0),
  _dutyNumer(1),
  _dutyDenom(4),
  _goodPumpno(goodPumpno),
  _badPumpno(badPumpno),
  _startSec(0),
  _startPumpGoodMsec(0),
  _startPumpBadMsec(0)
{
  Serial.println("# Turbido controller initialized");
}

int Tropho::begin(void)
{
  _startSec = rtcSeconds();
  _startPumpGoodMsec = pumpGood().totalOnMsec();
  _startPumpBadMsec =  pumpBad().totalOnMsec();

  setPumpNone();

  Serial.println("F\ttime.s\tneph\tgain\tstate\tgoodon\tbadon\tgood.s\tbad.s");

  return 0;
}

int Tropho::loop(void)
{
  delayOneSecond();

  long sec = rtcSeconds();
  long m = measure();

  const char *mode = "???";

  if (m < mLower()) {
    mode = "Lower";
    setPumpNone();
  } else if (m > mUpper()) {
    mode = "Upper";
    setPumpBad();
  } else if (dutyPump(sec)) {
    if (m >= mBad()) {
      mode = "High";
      setPumpBad();
    } else if (m <= mGood()) {
      mode = "Low";
      setPumpGood();
    } else {
      long cutoff = random(mGood()+1, mBad());
      if (m <= cutoff) {
        mode = "MidGood";
        setPumpGood();
      } else {
        mode = "MidBad";
        setPumpBad();
      }
    }
  } else {
    mode = "Wait";
    setPumpNone();
  }

  long ptimeGood = pumpGood().totalOnMsec(), ptimeBad = pumpBad().totalOnMsec();

  snprintf(Supervisor::outbuf, Supervisor::outbufLen, 
           "F\t%lu\t%ld\t%ld\t%s\t%d\t%d\t%ld.%03ld\t%ld.%03ld\r\n", 
           sec - _startSec, m, _s.nephelometer().pgaScale(), mode,
           pumpGood().isPumping(), pumpBad().isPumping(),
           ptimeGood / ((long) 1000), ptimeGood % ((long) 1000),
           ptimeBad / ((long) 1000), ptimeBad % ((long) 1000));
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

int Tropho::dutyPump(unsigned long runSeconds)
{
  if ((dutyDenom() < 2) || (dutyDenom() <= dutyNumer())) {
    return 1;
  } else {
    unsigned long periodSeconds = runSeconds % dutyDenom();

    if (dutyNumer() < 2) {
      return periodSeconds == 0;
    } else {
      unsigned long interval = (dutyDenom() - 1) / (dutyNumer() - 1);
      unsigned long periodIntervals = periodSeconds / interval;
      return ((periodSeconds % interval) == 0) && (periodIntervals < dutyNumer());
    }
  }
}

unsigned long Tropho::dutyFractionPercent(void)
{
  if ((dutyDenom() < 2) || (dutyDenom() < dutyNumer())) {
    return 100;
  } else {
    return (((unsigned long) 100) * dutyNumer()) / dutyDenom();
  }
}

void Tropho::formatParams(char *buf, unsigned int buflen)
{
  unsigned long dutyPct = dutyFractionPercent();
  snprintf(buf, buflen, "# Upper bound %ld\r\n# Pump bad    %ld\r\n# Pump good   %ld\r\n # Lower bound %ld\r\n# Duty %lu / %lu = %lu.%02lu\r\n",
           _mUpper, _mBad, _mGood, _mLower, _dutyNumer, _dutyDenom, dutyPct/100, dutyPct%100);
}

void Tropho::manualSetParams(void)
{
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));

  manualReadParam("Upper bound measurement  ", _mUpper);
  manualReadParam("All-bad pump measurement ", _mBad);
  manualReadParam("All-good pump measurement", _mGood);
  manualReadParam("Lower bound measurement  ", _mLower);

  if (_mGood <= _mLower) {
    Serial.println(F("# Require all-good pump > lower bound, correcting"));
    _mGood = _mLower + 1;
  }

  if (_mBad <= _mGood) {
    Serial.println(F("# Requre all-bad pump > all-good pump, correcting"));
    _mBad = _mGood + 1;
  }

  if (_mUpper <= _mBad) {
    Serial.println(F("# Require upper bound > all-bad pump, correcting"));
    _mUpper = _mBad + 1;
  }

  manualReadParam("Duty cycle numerator ", _dutyNumer);
  manualReadParam("Duty cycle denomiator", _dutyDenom);

  if (_dutyNumer < 1) {
    Serial.println(F("# Requre duty cycle >0 i.e. numer >= 1, correcting"));
    _dutyNumer = 1;
  }

  if (_dutyDenom <= _dutyNumer) {
    Serial.println(F("# Require duty cycle <1 i.e. denom > numer, correcting"));
    _dutyDenom = _dutyNumer + 1;
  }
  
  serialWriteParams();
}

