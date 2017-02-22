#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "turbidostat.h"

Turbido::Turbido(Nephel &neph, const NephelMeasure &measure, Pump &pump):
  _neph(neph),
  _measure(measure),
  _pump(pump),
  _pumpOn(0x7fffffff),
  _pumpOff(0),
  _startSec(0),
  _startPumpMsec(0)
{

}

int Turbido::begin(void)
{
  _startSec = rtcSeconds();
  _startPumpMsec = _pump.totalOnMsec();

  _pump.setPumping(0);

  return 0;
}

int Turbido::loop(void)
{
  long sec = rtcSeconds();
  long m = _neph.measure(_measure);

  if (_pump.isPumping() && m < _pumpOff) {
    _pump.setPumping(0);
  } else if ((!_pump.isPumping()) && m > _pumpOn) {
    _pump.setPumping(1);
  }

  long ptime = _pump.totalOnMsec();

  snprintf(Supervisor::outbuf, Supervisor::outbufLen, "T\t%lu\t%ld\t%ld.%03ld\r\n", 
           sec - _startSec, m, 
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
  _pumpOn = readEepromLong(eepromStart, 0);
  _pumpOff = readEepromLong(eepromStart, 1);
}

void Turbido::writeEeprom(unsigned int eepromStart)
{
  writeEepromLong(eepromStart, 0, _pumpOn);
  writeEepromLong(eepromStart, 1, _pumpOff);
}

void Turbido::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Pump on @ %ld\r\n# Pump off @ %ld\r\n", 
           _pumpOn, _pumpOff);
}

void Turbido::manualSetParams(void)
{
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));

  manualReadParam("pump on (high) measurement", _pumpOn);
  manualReadParam("pump off (low) measurement", _pumpOff);
  
  serialWriteParams();
}

