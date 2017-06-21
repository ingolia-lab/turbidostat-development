#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "turbidomix.h"

TurbidoMix::TurbidoMix(Supervisor &s):
  TurbidoBase(s),
  _pump1(0),
  _pump2(0),
  _pump1Pct(50),
  _cycleCount(0)
{

}

void TurbidoMix::formatHeader(char *buf, unsigned int buflen)
{
  strncpy(buf, "M\ttime.s\tneph\tgain\tpump1on\tpump2on\tpump1.s\tpump2.s", buflen);
}

int TurbidoMix::begin(void)
{
  _cycleCount = 0;
  return TurbidoBase::begin();
}

void TurbidoMix::formatLine(char *buf, unsigned int buflen, long m)
{
  long sec = rtcSeconds();
  long time1 = pump1().totalOnMsec(), time2 = pump2().totalOnMsec();

  snprintf(buf, buflen, 
           "U\t%lu\t%ld.%03ld\t%ld\t%d\t%d\t%ld.%03ld\t%ld.%03ld\r\n", 
           sec - startSec(), m / 1000, m % 1000, s().nephelometer().pgaScale(), 
           pump1().isPumping(), pump2().isPumping(),
           time1 / ((long) 1000), time1 % ((long) 1000),
           time2 / ((long) 1000), time2 % ((long) 1000));
}

Pump &TurbidoMix::pump1(void) { return s().pump(_pump1); }
Pump &TurbidoMix::pump2(void) { return s().pump(_pump2); }

void TurbidoMix::setPumpOn(void)
{
  uint8_t count = pumpCountIncr();

  if (schedulePercent(pump1Percent(), count)) {  
    setPump1On();
  } else {
    setPump2On();
  }
}

void TurbidoMix::setPump1On(void)  { pump1().setPumping(1); pump2().setPumping(0); }
void TurbidoMix::setPump2On(void)  { pump1().setPumping(0); pump2().setPumping(1); }
void TurbidoMix::setPumpOff(void)  { pump1().setPumping(0); pump2().setPumping(0); }

void TurbidoMix::formatParams(char *buf, unsigned int buflen)
{
  TurbidoBase::formatParams(buf, buflen);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "# Pump #1 %c\r\n# Pump #2 %c\r\n# Pump #1 %d%% share\r\n", 
           Supervisor::pumpnoToChar(_pump1), Supervisor::pumpnoToChar(_pump2), _pump1Pct);
}

void TurbidoMix::manualReadParams(void)
{
  manualReadPump("pump #1", _pump1);
  manualReadPump("pump #2", _pump2);
  manualReadPercent("pump #1 percentage", _pump1Pct);
}

