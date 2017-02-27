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

  Serial.println("U\ttime.s\tneph\tgain\tpumpAon\tpumpBon\tpumpA.s\tpumpB.s");

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

  long atime = pumpA().totalOnMsec(), btime = pumpB().totalOnMsec();

  snprintf(Supervisor::outbuf, Supervisor::outbufLen, 
           "U\t%lu\t%ld\t%ld\t%d\t%d\t%ld.%03ld\t%ld.%03ld\r\n", 
           sec - _startSec, m, _s.nephelometer().pgaScale(), 
           pumpA().isPumping(), pumpB().isPumping(),
           atime / ((long) 1000), atime % ((long) 1000),
           btime / ((long) 1000), btime % ((long) 1000));
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

long TurbidoMix::measure(void) { return _s.nephelometer().measure(); }

const Pump &TurbidoMix::pumpA(void) { return _s.pump(_pumpA); }
const Pump &TurbidoMix::pumpB(void) { return _s.pump(_pumpB); }

void TurbidoMix::setPumpA(void)     { _s.pump(_pumpA).setPumping(1); _s.pump(_pumpB).setPumping(0); }
void TurbidoMix::setPumpB(void)     { _s.pump(_pumpA).setPumping(0); _s.pump(_pumpB).setPumping(1); }
void TurbidoMix::setPumpNone(void)  { _s.pump(_pumpA).setPumping(0); _s.pump(_pumpB).setPumping(0); }

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

