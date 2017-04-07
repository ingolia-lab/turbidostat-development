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






/***************************StepTurbido child class********************************************************/
StepTurbido::StepTurbido(Supervisor &s, int pumpno):
  Turbido(s, pumpno),
  _stepLength(16200),
  _stepSize(10),
  _conversion(2000),
  _startTime(0),
  _stepMode(1),
  _step(1)
{
  Serial.println("# StepTurbido controller initialized");
}

int StepTurbido::begin(void)
{
  Turbido::begin();

  _startTime = getStartSec();
  _stepMode = 1;
  /* The following algorithm determines what the next step is based on step size and conversion.
   *  This prevents the Arduino from immediately running the pump in an attempt to dilute the culture.
   *  measure() is an IR measurement, divided by the conversion factor the user must provide. This determines
   *  the current fractional step of the culture. The casting and +1 turn this into a ceil()
   *  function, without importaint the math.h(in C++, idk the Arduino version) library
   */
  _step = _stepSize * (long)(int)(measure() / _conversion + 1); //This is done like this to avoid importing the math.h library.
  /* 1.05 and 0.95 give a +/- 5% range on the IR measurement. 
   *  at IR=1000, 5% = 50
   *  at IR = 40,000, 5% = 2000
   */
  setBounds(1.05 * _step/100 * _conversion, 0.95 * _step/100 * _conversion);

  return 0;
}

int StepTurbido::loop(void)
{
  long sec = rtcSeconds();

  if (_stepMode == 1)
  {
    if (sec - _startTime >= _stepLength)
    {
      _step = _step + _stepSize;    //Set new step as one stepSize bigger than the previous

      setBounds(1.05 * _step/100 * _conversion, 0.95 * _step/100 * _conversion);    //Set new measurement bounds to run the pump.
      _startTime = sec;   //reset cycle time.

      if ( (_step + _stepSize) > 100) //Stop stepping at OD=1.0
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
           "ST\t%lu\t%ld\t\t%ld.%02ld\t\t%lu\t%ld\t%d\t%ld.%03ld\r\n",
           sec - getStartSec(), m, _step/100, _step%100, sec - _startTime, getPGAScale(), pump().isPumping(),
           ptime / ((long) 1000), ptime % ((long) 1000));
  Serial.write(Supervisor::outbuf);
}

void StepTurbido::readEeprom(unsigned int eepromStart)
{
  _stepLength = readEepromLong(eepromStart, 0);
  _stepSize = readEepromLong(eepromStart, 1);
  _conversion = readEepromLong(eepromStart, 2);
  setPumpNo(readEepromLong(eepromStart, 3));
}

void StepTurbido::writeEeprom(unsigned int eepromStart)
{
  writeEepromLong(eepromStart, 0, _stepLength);
  writeEepromLong(eepromStart, 1, _stepSize);
  writeEepromLong(eepromStart, 2, _conversion);
  writeEepromLong(eepromStart, 3, getPumpNo());
}

void StepTurbido::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Step time @ %ld\r\n# Step size @ %ld.%02ld\r\n# 0.1 OD->IR Conversion @ %ld\r\n# Pump number %ld\r\n",
           _stepLength, _stepSize/100, _stepSize%100, _conversion, getPumpNo());
}
void StepTurbido::manualSetParams(void)
{
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));

  manualReadParam("time spent at each step   ", _stepLength);
  manualReadParam("percent step size         ", _stepSize);
  manualReadParam("0.1 OD->IR conversion     ", _conversion);

  long ch = getPumpNo();
  manualReadParam("pump number               ", ch);
  setPumpNo((int)ch);

  serialWriteParams();
}
/*ENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDEND*/
