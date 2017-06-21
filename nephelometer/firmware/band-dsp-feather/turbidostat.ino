#include "controller.h"
#include "pump.h"
#include "supervisor.h"
#include "turbidobase.h"
#include "turbidostat.h"

Turbidostat::Turbidostat(Supervisor &s):
  TurbidoBase(s),
  _pumpno(0)
{

}

void Turbidostat::formatHeader(char *buf, unsigned int buflen)
{
  strncpy(buf, "T\ttime.s\tneph\tgain\tpumpon\tpumptime.s", buflen);
}

void Turbidostat::formatLine(char *buf, unsigned int buflen, long m)
{
  long sec = rtcSeconds();
  long ptime = pump().totalOnMsec();

  snprintf(buf, buflen, "T\t%lu\t%ld.%03ld\t%ld\t%d\t%ld.%03ld", 
           sec - startSec(), m/1000, m%1000, s().nephelometer().pgaScale(), pump().isPumping(),
           ptime / ((long) 1000), ptime % ((long) 1000));
}

Pump &Turbidostat::pump(void) { return s().pump(_pumpno); }

void Turbidostat::setPumpOn(void)  { pump().setPumping(1); }

void Turbidostat::setPumpOff(void) { pump().setPumping(0); }

void Turbidostat::formatParams(char *buf, unsigned int buflen)
{
  TurbidoBase::formatParams(buf, buflen);
  snprintf(buf + strlen(buf), buflen - strlen(buf),
           "# Pump %c\r\n", 
           Supervisor::pumpnoToChar(_pumpno));
}

void Turbidostat::manualReadParams(void)
{
  TurbidoBase::manualReadParams();
  manualReadPump("media pump", _pumpno);
}

