#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "turbidostat.h"

Turbido::Turbido(Nephel &neph, Pump &pump):
  _neph(neph),
  _pump(pump),
  _mUpper(0x7fffffff),
  _mLower(0),
  _startSec(0),
  _startPumpMsec(0)
{
  Serial.println("# Turbido controller initialized");
}

int Turbido::begin(void)
{
  _startSec = rtcSeconds();
  _startPumpMsec = _pump.totalOnMsec();

  _pump.setPumping(0);

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
  } else if ((!_pump.isPumping()) && m > mUpper()) {
    setPumpOn();
  }

  long ptime = pump().totalOnMsec();

  snprintf(Supervisor::outbuf, Supervisor::outbufLen, 
           "T\t%lu\t%ld\t%ld\t%d\t%ld.%03ld\r\n", 
           sec - _startSec, m, _neph.pgaScale(), pump().isPumping(),
           ptime / ((long) 1000), ptime % ((long) 1000));
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

void Turbido::readEeprom(unsigned int eepromStart)
{
  _mUpper = readEepromLong(eepromStart, 0);
  _mLower = readEepromLong(eepromStart, 1);
}

void Turbido::writeEeprom(unsigned int eepromStart)
{
  writeEepromLong(eepromStart, 0, _mUpper);
  writeEepromLong(eepromStart, 1, _mLower);
}

void Turbido::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Pump on @ %ld\r\n# Pump off @ %ld\r\n", 
           _mUpper, _mLower);
}

void Turbido::manualSetParams(void)
{
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));

  manualReadParam("pump on (high) measurement", _mUpper);
  manualReadParam("pump off (low) measurement", _mLower);
  
  serialWriteParams();
}

