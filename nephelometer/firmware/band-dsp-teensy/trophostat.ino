#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "trophostat.h"

Tropho::Tropho(Nephel &neph, Pump &pumpGood, Pump &pumpBad):
  _neph(neph),
  _pumpGood(pumpGood),
  _pumpBad(pumpBad),
  _mUpper(0x7fffffff),
  _mTarget(2000),
  _mLower(0),
  _dutyNumer(1),
  _dutyDenom(4),
  _startSec(0),
  _startPumpGoodMsec(0),
  _startPumpBadMsec(0)
{
  Serial.println("# Turbido controller initialized");
}

int Tropho::begin(void)
{
  _startSec = rtcSeconds();
  _startPumpGoodMsec = _pumpGood.totalOnMsec();
  _startPumpBadMsec =  _pumpBad.totalOnMsec();

  _pumpGood.setPumping(0);
  _pumpBad.setPumping(0);

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
    if (m < mTarget()) {
      mode = "Low";
      setPumpGood();
    } else {
      mode = "High";
      setPumpBad();
    }
  } else {
    mode = "Wait";
    setPumpNone();
  }

  long ptimeGood = pumpGood().totalOnMsec(), ptimeBad = pumpBad().totalOnMsec();

  snprintf(Supervisor::outbuf, Supervisor::outbufLen, 
           "F\t%lu\t%ld\t%ld\t%s\t%d\t%d\t%ld.%03ld\t%ld.%03ld\r\n", 
           sec - _startSec, m, _neph.pgaScale(), mode,
           pumpGood().isPumping(), pumpBad().isPumping(),
           ptimeGood / ((long) 1000), ptimeGood % ((long) 1000),
           ptimeBad / ((long) 1000), ptimeBad % ((long) 1000));
  Serial.write(Supervisor::outbuf);

  int ch;
  while ((ch = Serial.read()) >= 0) {
    if (ch == 'q') {
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

void Tropho::readEeprom(unsigned int eepromStart)
{
  _mUpper = readEepromLong(eepromStart, 0);
  _mTarget = readEepromLong(eepromStart, 1);
  _mLower = readEepromLong(eepromStart, 2);
  _dutyNumer = (unsigned long) readEepromLong(eepromStart, 3);
  _dutyDenom = (unsigned long) readEepromLong(eepromStart, 4);
}

void Tropho::writeEeprom(unsigned int eepromStart)
{
  writeEepromLong(eepromStart, 0, _mUpper);
  writeEepromLong(eepromStart, 1, _mTarget);
  writeEepromLong(eepromStart, 2, _mLower);
  writeEepromLong(eepromStart, 3, (long) _dutyNumer);
  writeEepromLong(eepromStart, 4, (long) _dutyDenom);
}

void Tropho::formatParams(char *buf, unsigned int buflen)
{
  unsigned long dutyPct = dutyFractionPercent();
  snprintf(buf, buflen, "# Upper bound %ld\r\n# Target      %ld\r\n# Lower bound %ld\r\n# Duty %lu / %lu = %lu.%02lu\r\n",
           _mUpper, _mTarget, _mLower, _dutyNumer, _dutyDenom, dutyPct/100, dutyPct%100);
}

void Tropho::manualSetParams(void)
{
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));

  manualReadParam("Upper bound measurement", _mUpper);
  manualReadParam("Target measurement     ", _mTarget);
  manualReadParam("Lower bound measurement", _mLower);

  manualReadParam("Duty cycle numerator ", _dutyNumer);
  manualReadParam("Duty cycle denomiator", _dutyDenom);
  
  serialWriteParams();
}

