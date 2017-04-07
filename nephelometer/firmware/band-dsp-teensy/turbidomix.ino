#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "turbidomix.h"

TurbidoMix::TurbidoMix(Supervisor &s):
  _s(s),
  _mUpper(0x7fffffff),
  _mLower(0),
  _pumpA(0),
  _pumpB(1),
  _pumpAShare(1),
  _pumpBShare(1),  
  _startSec(0),
  _startPumpAMsec(0),
  _startPumpBMsec(0),
  _cycleCount(0)
{
  Serial.println("# TurbidoMix controller initialized");
}

int TurbidoMix::begin(void)
{
  _startSec = rtcSeconds();
  _startPumpAMsec = pumpA().totalOnMsec();
  _startPumpBMsec = pumpB().totalOnMsec();

  setPumpNone();

  _cycleCount = 0;

  printHeader();

  return 0;
}

int TurbidoMix::loop(void)
{
  delayOneSecond();

  long sec = rtcSeconds();  //may not need?
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

  printStatus(m);

  int ch;
  while ((ch = Serial.read()) >= 0) {
    if (ch == 'q') {
      setPumpNone();  //JBB, 2017_03_20. Stop pumps when program is stopped.
      return 1;
      while (Serial.read() >= 0) {
        /* DISCARD */ 
      }
    } 
  }

  return 0;
}

long TurbidoMix::measure(void) { return _s.nephelometer().measure(); }

const Pump &TurbidoMix::pumpA(void) { return _s.pump(_pumpA); }
const Pump &TurbidoMix::pumpB(void) { return _s.pump(_pumpB); }

void TurbidoMix::setPumpA(void)     { _s.pump(_pumpA).setPumping(1); _s.pump(_pumpB).setPumping(0); }
void TurbidoMix::setPumpB(void)     { _s.pump(_pumpA).setPumping(0); _s.pump(_pumpB).setPumping(1); }
void TurbidoMix::setPumpNone(void)  { _s.pump(_pumpA).setPumping(0); _s.pump(_pumpB).setPumping(0); }

void TurbidoMix::printHeader(void)
{
  Serial.println("U\ttime.s\tneph\tgain\tpumpAon\tpumpBon\tpumpA.s\tpumpB.s");
}

void TurbidoMix::printStatus(long m)
{
  long sec = rtcSeconds();
  long atime = pumpA().totalOnMsec(), btime = pumpB().totalOnMsec();

  snprintf(Supervisor::outbuf, Supervisor::outbufLen, 
           "U\t%lu\t%ld\t%ld\t%d\t%d\t%ld.%03ld\t%ld.%03ld\r\n", 
           sec - _startSec, m, getPGAScale(), 
           pumpA().isPumping(), pumpB().isPumping(),
           atime / ((long) 1000), atime % ((long) 1000),
           btime / ((long) 1000), btime % ((long) 1000));
  Serial.write(Supervisor::outbuf);
}

void TurbidoMix::setPumpOnCycle(void)
{
  unsigned long count = pumpCountIncr();
  if ((count % pumpCycle()) < ((unsigned long) _pumpAShare)) {
    setPumpA();
  } else {
    setPumpB();
  }
}

void TurbidoMix::readEeprom(unsigned int eepromStart)
{
  _mUpper = readEepromLong(eepromStart, 0);
  _mLower = readEepromLong(eepromStart, 1);
  _pumpA = readEepromLong(eepromStart, 2);
  _pumpB = readEepromLong(eepromStart, 3);
  _pumpAShare = readEepromLong(eepromStart, 4);
  _pumpBShare = readEepromLong(eepromStart, 5);
}

void TurbidoMix::writeEeprom(unsigned int eepromStart)
{
  writeEepromLong(eepromStart, 0, _mUpper);
  writeEepromLong(eepromStart, 1, _mLower);
  writeEepromLong(eepromStart, 2, _pumpA);
  writeEepromLong(eepromStart, 3, _pumpB);
  writeEepromLong(eepromStart, 4, _pumpAShare);
  writeEepromLong(eepromStart, 5, _pumpBShare);
}

void TurbidoMix::formatParams(char *buf, unsigned int buflen)
{
  long a1k = (1000 * _pumpAShare) / (_pumpAShare + _pumpBShare);
  long b1k = (1000 * _pumpBShare) / (_pumpAShare + _pumpBShare);
  
  snprintf(buf, buflen, "# Pump on @ %ld\r\n# Pump off @ %ld\r\n# Pump A %ld share %ld (%ld.%03ld)\r\n# Pump B %ld share %ld (%ld.%03ld)\r\n", 
           _mUpper, _mLower, 
           _pumpA, _pumpAShare, a1k/1000, a1k%1000,
           _pumpB, _pumpBShare, b1k/1000, b1k%1000);
}

void TurbidoMix::manualSetParams(void)
{
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));

  manualReadParam("pump on (high) measurement", _mUpper);
  manualReadParam("pump off (low) measurement", _mLower);
  manualReadParam("pump A number             ", _pumpA);
  manualReadParam("pump B number             ", _pumpB);
  manualReadParam("pump A share              ", _pumpAShare);
  manualReadParam("pump B share              ", _pumpBShare);
  
  serialWriteParams();
}













/***************************StepTurbidoMix child class********************************************************/
StepTurbidoMix::StepTurbidoMix(Supervisor &s):
    TurbidoMix(s),
  _stepLength(16200),   //16200 = 3 doubling times, 1.5 hrs per generation (based on yeast), in units of seconds.
  _startTime(0),
  _stepRate(71) //a rate of 71/100 gives pumpA shares of: 100, 71, 50, 35, 24, 17, 12, 8, 5, 3, 2, 1, 0.
{
  setPumpShares(100,0);   //set the pump shares to PumpA=100, pumpB=0. This works out well in the math later.
  Serial.println("# StepTurbidoMix controller initialized");
}

int StepTurbidoMix::begin(void)
{
  TurbidoMix::begin();
  _startTime = getStartSec();   

  return 0;
}

int StepTurbidoMix::loop(void)
{
  long sec = rtcSeconds();

/*This is the key to the stepping portion. It checks the time of the cycle, and if the length has passed, it
 * steps the pumping fraction one round forward. It then updates _startTime to begin a new cycle. 
 * Since it steps down, it will quit moving when pumpA is 0, but the loop will continue going. This is effectively
 * a quit, without writing an explicit exit to the stepping. 
 */
  if (sec - _startTime >= _stepLength)
  {
    long newA = (long)(int)(getPumpAShare()*_stepRate/100);   //Casting should keep pump shares at integer values. 

    //long newA = (long)(int)(getPumpAShare()*_stepRate/100+0.5);
    /*  This formula rounds the answer instead of straight truncating. 
     *  for _stepRate = 71/100, with rounding, we get the series:
     *  100, 71, 50, 36, 26, 18, 13, 9, 6, 4, 3, 2, 1, 0.
     *  This series is 1 step longer than the truncated series, adding 
     *  a step of 4:96 A:B ratio, amongst other slight changes. 
     */
          
    setPumpShares(newA, pumpCycle()-newA);           
    
    if( (newA-_stepRate/100) >= 0) 
    {
        _startTime = sec;
    }  
  }

  return TurbidoMix::loop(); 
}

void StepTurbidoMix::printHeader(void)
{
  Serial.println("V\ttime.s\tneph\tpumpA_Share\tpumpA_Frac\tstep_time.s\tgain\tpumpAon\tpumpBon\tpumpA.s\tpumpB.s");
}

void StepTurbidoMix::printStatus(long m)
{
  long sec = rtcSeconds();
  long atime = pumpA().totalOnMsec(), btime = pumpB().totalOnMsec();

  snprintf(Supervisor::outbuf, Supervisor::outbufLen, 
           "V\t%lu\t%ld\t%ld\t\t%ld.%02ld\t\t%lu\t\t%ld\t%d\t%d\t%ld.%03ld\t%ld.%03ld\r\n", 
           sec - getStartSec(), m, getPumpAShare(), getPumpAShare()/100, getPumpAShare()%100, sec - _startTime, getPGAScale(), 
           pumpA().isPumping(), pumpB().isPumping(),
           atime / ((long) 1000), atime % ((long) 1000),
           btime / ((long) 1000), btime % ((long) 1000));
  Serial.write(Supervisor::outbuf);
}  

void StepTurbidoMix::readEeprom(unsigned int eepromStart)
{
  TurbidoMix::readEeprom(eepromStart);    //Calls the default parameters, which StepTurbidoMix still uses.
  _stepLength = readEepromLong(eepromStart, 6);   //Sets the new parameters here and in the line below.
  _stepRate = readEepromLong(eepromStart, 7);
}

void StepTurbidoMix::writeEeprom(unsigned int eepromStart)
{
  TurbidoMix::writeEeprom(eepromStart);   //Writes the default parameters. This way the user can see where all the 
                                          //settings begin. Even the ones they can't change.
  writeEepromLong(eepromStart, 6, _stepLength); //writes new parameters here and below
  writeEepromLong(eepromStart, 7, _stepRate);
}

void StepTurbidoMix::formatParams(char *buf, unsigned int buflen)
{
  long a1k = (1000 * getPumpAShare()) / (getPumpAShare() + getPumpBShare());
  long b1k = (1000 * getPumpBShare()) / (getPumpAShare() + getPumpBShare());
  
  snprintf(buf, buflen, "# Pump on @ %ld\r\n# Pump off @ %ld\r\n# Pump A %ld share %ld (%ld.%03ld)\r\n# Pump B %ld share %ld (%ld.%03ld)\r\n# Step time %ld\r\n# Step rate 0.%02ld\r\n", 
           mUpper(), mLower(), 
           getPumpA(), getPumpAShare(), a1k/1000, a1k%1000,
           getPumpB(), getPumpBShare(), b1k/1000, b1k%1000,
           _stepLength, _stepRate%100);
}

void StepTurbidoMix::manualSetParams(void)
{
  Serial.print(F("# PumpA share begins at 100 and steps down\r\n"));  //Tells the user where it begins. Need to change the 100 to a variable? 
                                                                      //It's currently hard coded in two places
  Serial.print(F("# PumpB share begins at 0 and steps up\r\n"));
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));


  long ch = mUpper();
  manualReadParam("pump on (high) measurement", ch);
  setmUpper(ch);

  ch = mLower();
  manualReadParam("pump off (low) measurement", ch);
  setmLower(ch);

  ch = getPumpA();
  manualReadParam("pump A number             ", ch);
  setNumPumpA(ch);

  ch = getPumpB();
  manualReadParam("pump B number             ", ch);
  setNumPumpB(ch);
  
  manualReadParam("time spent at each step   ", _stepLength);
  manualReadParam("percent step rate  ", _stepRate);
  
  serialWriteParams();
}

/*ENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDENDEND*/
