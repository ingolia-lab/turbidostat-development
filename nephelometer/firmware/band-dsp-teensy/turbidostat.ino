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
  _startPumpMsec(0),
  _stepMode(0),    //JBB, 2017_03_27
  _time(0), //JBB, 2017_03_27
  _sLower(0), //JBB, 2017_03_27
  _sUpper(0)  //JBB, 2017_03_27
{
  Serial.println("# Turbido controller initialized");
}

int Turbido::begin(void)
{
  _startSec = rtcSeconds();
  _startPumpMsec = pump().totalOnMsec();

  setPumpOff();
  
  //JBB, 2017_02_27. Add a stepping mode to the turbidostat. 
  Serial.println("# Would you like to enter stepping mode? (y/n)");
    int ch;
    while ((ch = Serial.read()) < 0) {
      delay(1);
    }
  
  Serial.print("# ");
  Serial.write(ch);
    
  if(ch=='y')
  {
    Serial.print("= ");
    Serial.println("Stepping mode");
    _stepMode=1;
    _time=rtcSeconds();
    _sLower=mLower();
    _sUpper=mUpper();
  }
  else if(ch=='n')
  {
    Serial.print("= ");
    Serial.println("Normal Mode");
    _stepMode=0;
  }
  else
  {
    Serial.println("!= 'y' or 'n'");
    Serial.print(F("# Not entering stepping mode\r\n"));
    _stepMode=0;
  }

  Serial.println("T\ttime.s\tneph\tgain\tpumpon\tpumptime.s");

  return 0;
}

int Turbido::loop(void)
{
  delayOneSecond();

  long sec = rtcSeconds();
  long m = measure();

  //JBB, 2017_03_27. Create a stepping mode within the Turbidostat. 
  if (_stepMode==1)
  {
    if(sec-_time >= 14400)  //Hardcoded to change every 4 hours right now. Could make this a setable field. 
    {
      _mLower = _mLower + 0.95*_sLower; //gives a 5% buffer on each increase.
      _mUpper = _mUpper + 1.05*_sUpper; //gives a 5% buffer on each increase
      _time = sec;
      
      //Hardecode 5 steps, which is 4.8*_sLower and 5.4*_sUpper. At this point, stop stepping.
      if(_mLower>4.5*_sLower || _mUpper>5*_sUpper)
      {
        _stepMode = 0;  //This will quit the stepping. 
      }
    }
  }
  
  //JBB, 2017_03_27. Changed mLower() and mUpper() to _mLower and _mUpper
  if (pump().isPumping() && m < _mLower) {
    setPumpOff();
  } else if ((!pump().isPumping()) && m > _mUpper) {
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

