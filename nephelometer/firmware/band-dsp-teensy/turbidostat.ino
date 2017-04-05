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

  printHeader();

  return 0;
}

int Turbido::loop(void)
{
  delayOneSecond();

  long m = measure();

  if (pump().isPumping() && m < mLower()) {
    setPumpOff();
  } else if ((!pump().isPumping()) && m > mUpper()) {
    setPumpOn();
  }

  printStatus(m);

  int ch;
  while ((ch = Serial.read()) >= 0) {
    if (ch == 'q') {
      setPumpOff();  //JBB, 2017_03_20. Stop pumps when program is stopped.
      return 1;
      while (Serial.read() >= 0) {
        /* DISCARD */
      }
    }
  }

  return 0;
}

void Turbido::printHeader(void)
{
  Serial.println("T\ttime.s\tneph\tgain\tpumpon\tpumptime.s");
}

void Turbido::printStatus(long m)
{
  long sec = rtcSeconds();
  long ptime = pump().totalOnMsec();

  snprintf(Supervisor::outbuf, Supervisor::outbufLen,
           "T\t%lu\t%ld\t%ld\t%d\t%ld.%03ld\r\n",
           sec - getStartSec(), m, getPGAScale(), pump().isPumping(),
           ptime / ((long) 1000), ptime % ((long) 1000));
  Serial.write(Supervisor::outbuf);
}

long Turbido::measure(void) {
  return _s.nephelometer().measure();
}

const Pump &Turbido::pump(void) {
  return _s.pump(_pumpno);
}

void Turbido::setPumpOn(void)  {
  _s.pump(_pumpno).setPumping(1);
}

void Turbido::setPumpOff(void) {
  _s.pump(_pumpno).setPumping(0);
}

long Turbido::getPGAScale(void) {
  return _s.nephelometer().pgaScale();
}

void Turbido::readEeprom(unsigned int eepromStart)
{
  setBounds(readEepromLong(eepromStart, 0), readEepromLong(eepromStart, 1));
  setPumpNo(readEepromLong(eepromStart, 2));
}

void Turbido::writeEeprom(unsigned int eepromStart)
{
  writeEepromLong(eepromStart, 0, _mUpper);
  writeEepromLong(eepromStart, 1, _mLower);
  writeEepromLong(eepromStart, 2, getPumpNo());
}

void Turbido::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Pump on @ %ld\r\n# Pump off @ %ld\r\n# Pump number %ld\r\n",
           mUpper(), mLower(), getPumpNo());
}

void Turbido::manualSetParams(void)
{
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));

  manualReadParam("pump on (high) measurement", _mUpper);
  manualReadParam("pump off (low) measurement", _mLower);

  long ch = getPumpNo();
  manualReadParam("pump number               ", ch );
  setPumpNo((int)ch);

  serialWriteParams();
}






/*define StepTurbido class things down here*/
StepTurbido::StepTurbido(Supervisor &s, int pumpno):
  Turbido(s, pumpno),
  _stepLength(0),
  _stepSize(0),
  _conversion(0),
  _startTime(0),
  _stepMode(0),
  _step(0)

{
  Serial.println("# StepTurbido controller initialized");
}

int StepTurbido::begin(void)
{
  Turbido::begin();

  _startTime = getStartSec();
  _stepMode = 1;
  /*Determine the NEXT closest step to start on*/
  _step = _stepSize * (long)(int)(measure() / _conversion + 1); //This is done like this to avoid importing the math.h library.

  setBounds(1.05 * _step * _conversion, 0.95 * _step * _conversion);

  return 0;
}

int StepTurbido::loop(void)
{
  long sec = rtcSeconds();

  if (_stepMode == 1)
  {
    if (sec - _startTime >= _stepLength)
    {
      _step = _step + _stepSize;

      setBounds(1.05 * _step * _conversion, 0.95 * _step * _conversion);
      _startTime = sec;

      if (_step * _stepSize >= 1.0) //Stop stepping at OD=1.0
      {
        _stepMode = 0;  //This will quit the stepping.
      }
    }
  }

  return Turbido::loop();
}

void StepTurbido::printHeader(void)
{
  Serial.println("ST\ttime.s\tneph\ttarget_O.D.\tstep_time.s\tgain\tpumpon\tpumptime.s");
}

void StepTurbido::printStatus(long m)
{
  long sec = rtcSeconds();
  long ptime = pump().totalOnMsec();

  snprintf(Supervisor::outbuf, Supervisor::outbufLen,
           "ST\t%lu\t%ld\t%ld\t%lu\t%ld\t%d\t%ld.%03ld\r\n",
           sec - getStartSec(), m, _step, sec - _startTime, getPGAScale(), pump().isPumping(),
           ptime / ((long) 1000), ptime % ((long) 1000));
  Serial.write(Supervisor::outbuf);
}

void StepTurbido::readEeprom(unsigned int eepromStart)
{
  _stepLength = readEepromLong(eepromStart, 16200);
  _stepSize = readEepromLong(eepromStart, 0.1);
  _conversion = readEepromLong(eepromStart, 2000);
  setPumpNo(readEepromLong(eepromStart, 2));
}

void StepTurbido::writeEeprom(unsigned int eepromStart)
{
  writeEepromLong(eepromStart, 16200, _stepLength);
  writeEepromLong(eepromStart, 0.1, _stepSize);
  writeEepromLong(eepromStart, 2000, _conversion);
  writeEepromLong(eepromStart, 2, getPumpNo());
}

void StepTurbido::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Step time @ %ld\r\n# Step size @ %ld\r\n# OD->IR Conversion @ %ld\r\n# Pump number %ld\r\n",
           _stepLength, _stepSize, _conversion, getPumpNo());
}
void StepTurbido::manualSetParams(void)
{
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));

  manualReadParam("time spent at each step   ", _stepLength);
  manualReadParam("step size                 ", _stepSize);
  manualReadParam("0.1 OD to __IR conversion ", _conversion);

  long ch = getPumpNo();
  manualReadParam("pump number               ", ch);
  setPumpNo((int)ch);

  serialWriteParams();
}
/*ENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDEND*/
