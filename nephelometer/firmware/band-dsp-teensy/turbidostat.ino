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
           "T\t%lu\t%ld\t%ld\t%d\t%ld.%03ld\r\n", 
           sec - _startSec, m, _s.nephelometer().pgaScale(), pump().isPumping(),
           ptime / ((long) 1000), ptime % ((long) 1000));
  Serial.write(Supervisor::outbuf);

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

long Turbido::measure(void) { return _s.nephelometer().measure(); }

const Pump &Turbido::pump(void) { return _s.pump(_pumpno); }

void Turbido::setPumpOn(void)  { _s.pump(_pumpno).setPumping(1); }

void Turbido::setPumpOff(void) { _s.pump(_pumpno).setPumping(0); }

void Turbido::readEeprom(unsigned int eepromStart)
{
  _mUpper = readEepromLong(eepromStart, 0);
  _mLower = readEepromLong(eepromStart, 1);
  _pumpno = readEepromLong(eepromStart, 2);
}

void Turbido::writeEeprom(unsigned int eepromStart)
{
  writeEepromLong(eepromStart, 0, _mUpper);
  writeEepromLong(eepromStart, 1, _mLower);
  writeEepromLong(eepromStart, 2, _pumpno);
}

void Turbido::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Pump on @ %ld\r\n# Pump off @ %ld\r\n# Pump number %ld\r\n", 
           _mUpper, _mLower, _pumpno);
}

void Turbido::manualSetParams(void)
{
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));

  manualReadParam("pump on (high) measurement", _mUpper);
  manualReadParam("pump off (low) measurement", _mLower);
  manualReadParam("pump number               ", _pumpno);
  
  serialWriteParams();
}




















/*define StepTurbido class things down here*/
StepTurbido::StepTurbido(Supervisor &s, int pumpno):
  _s(s),
  _mUpper(0),
  _mLower(0),
  _pumpno(pumpno),
  _startSec(0),
  _startPumpMsec(0),
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
  _startSec = rtcSeconds();
  _startPumpMsec = pump().totalOnMsec();
  _startTime = _startSec;
  _stepMode = 1;
  /*Determine the NEXT closest step to start on*/
  _step = _stepSize*(long)(int)(measure()/_conversion+1); //This is done like this to avoid importing the math.h library.
  _mLower = 0.95*_step*_conversion;
  _mUpper = 1.05*_step*_conversion;
  setPumpOff();  

  Serial.println("ST\ttime.s\tneph\ttarget_O.D.\tstep_time.s\tgain\tpumpon\tpumptime.s");

  return 0;
}

int StepTurbido::loop(void)
{
  delayOneSecond();

  long sec = rtcSeconds();
  long m = measure();

  if(_stepMode == 1)
  {
    if(sec - _startTime >= _stepLength)   
    {
      _step = _step+_stepSize;
      
      _mLower = 0.95*_step*_conversion; //gives a 5% buffer on each increase.
      _mUpper = 1.05*_step*_conversion; //gives a 5% buffer on each increase
      _startTime = sec;
      
      if(_step*_stepSize >= 1.0)  //Stop stepping at OD=1.0
      {
        _stepMode = 0;  //This will quit the stepping. 
      }
    }
  }
  
  if (pump().isPumping() && m < _mLower) {
    setPumpOff();
  } else if ((!pump().isPumping()) && m > _mUpper) {
    setPumpOn();
  }

  long ptime = pump().totalOnMsec();

  snprintf(Supervisor::outbuf, Supervisor::outbufLen, 
           "ST\t%lu\t%ld\t%ld\t%lu\t%ld\t%d\t%ld.%03ld\r\n", 
           sec - _startSec, m, _step, sec-_startTime, _s.nephelometer().pgaScale(), pump().isPumping(),
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

void StepTurbido::readEeprom(unsigned int eepromStart)
{
  _stepLength = readEepromLong(eepromStart, 3600);
  _stepSize = readEepromLong(eepromStart, 0.1);
  _conversion = readEepromLong(eepromStart, 2000); 
  _pumpno = readEepromLong(eepromStart, 2);
}

void StepTurbido::writeEeprom(unsigned int eepromStart)
{
  writeEepromLong(eepromStart, 3600, _stepLength);
  writeEepromLong(eepromStart, 0.1, _stepSize);
  writeEepromLong(eepromStart, 2000, _conversion);
  writeEepromLong(eepromStart, 2, _pumpno);
}

void StepTurbido::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Step time @ %ld\r\n# Step size @ %ld\r\n# OD->IR Conversion @ %ld\r\n# Pump number %ld\r\n", 
           _stepLength, _stepSize, _conversion, _pumpno);
}
void StepTurbido::manualSetParams(void)
{
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));

  manualReadParam("time spent at each step   ", _stepLength);                                     
  manualReadParam("step size                 ", _stepSize);
  manualReadParam("0.1 OD to __IR conversion ", _conversion);
  manualReadParam("pump number               ", _pumpno);
  
  serialWriteParams();
}

















 

