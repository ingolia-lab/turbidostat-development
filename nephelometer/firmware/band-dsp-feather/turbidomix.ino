#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "turbidomix.h"

TurbidoMixBase::TurbidoMixBase(Supervisor &s):
  TurbidoBase(s),
  _pump1(0),
  _pump2(0),
  _cycleCount(0)
{

}

void TurbidoMixBase::formatHeader(char *buf, unsigned int buflen)
{
  strncpy(buf, "\ttime.s\tneph\tgain\tpump1on\tpump2on\tpump1.s\tpump2.s", buflen);
}

int TurbidoMixBase::begin(void)
{
  _cycleCount = 0;
  return TurbidoBase::begin();
}

void TurbidoMixBase::formatLine(char *buf, unsigned int buflen, long m)
{
  long sec = rtcSeconds();
  long time1 = pump1().totalOnMsec(), time2 = pump2().totalOnMsec();

  snprintf(buf, buflen, 
           "\t%lu\t%ld.%03ld\t%ld\t%d\t%d\t%ld.%03ld\t%ld.%03ld", 
           sec - startSec(), m / 1000, m % 1000, s().nephelometer().pgaScale(), 
           pump1().isPumping(), pump2().isPumping(),
           time1 / ((long) 1000), time1 % ((long) 1000),
           time2 / ((long) 1000), time2 % ((long) 1000));
}

Pump &TurbidoMixBase::pump1(void) { return s().pump(_pump1); }
Pump &TurbidoMixBase::pump2(void) { return s().pump(_pump2); }

void TurbidoMixBase::setPumpOn(void)
{
  uint8_t count = pumpCountIncr();

  if (schedulePercent(pump1Percent(), count)) {  
    setPump1On();
  } else {
    setPump2On();
  }
}

void TurbidoMixBase::setPump1On(void)  { pump1().setPumping(1); pump2().setPumping(0); }
void TurbidoMixBase::setPump2On(void)  { pump1().setPumping(0); pump2().setPumping(1); }
void TurbidoMixBase::setPumpOff(void)  { pump1().setPumping(0); pump2().setPumping(0); }

void TurbidoMixBase::formatParams(char *buf, unsigned int buflen)
{
  TurbidoBase::formatParams(buf, buflen);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "# Pump #1 %c\r\n# Pump #2 %c\r\n", 
           Supervisor::pumpnoToChar(_pump1), Supervisor::pumpnoToChar(_pump2));
}

void TurbidoMixBase::manualReadParams(void)
{
  TurbidoBase::manualReadParams();
  manualReadPump("pump #1", _pump1);
  manualReadPump("pump #2", _pump2);
}

TurbidoMixFixed::TurbidoMixFixed(Supervisor &s):
  TurbidoMixBase(s),
  _pump1Pct(50)
{

}

void TurbidoMixFixed::formatHeader(char *buf, unsigned int buflen)
{
  buf[0] = 'M';
  TurbidoMixBase::formatHeader(buf + 1, buflen - 1);
}

void TurbidoMixFixed::formatLine(char *buf, unsigned int buflen, long m)
{
  buf[0] = 'M';
  TurbidoMixBase::formatLine(buf, buflen, m);
}

void TurbidoMixFixed::formatParams(char *buf, unsigned int buflen)
{
  TurbidoMixBase::formatParams(buf, buflen);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "# Pump #1 percentage %d%%\r\n", 
           _pump1Pct);
}

void TurbidoMixFixed::manualReadParams(void)
{
  TurbidoMixBase::manualReadParams();
  manualReadPercent("pump #1 percentage", _pump1Pct);
}

