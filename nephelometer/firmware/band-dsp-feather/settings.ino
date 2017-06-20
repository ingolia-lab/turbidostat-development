#include "settings.h"
#include "supervisor.h"

void ParamSettings::manualReadParam(const char *desc, long &pval)
{
  long vnew;
  Serial.print("# Enter ");
  Serial.print(desc);
  Serial.print("(");
  Serial.print(pval);
  Serial.print("): ");
  if (Supervisor::blockingReadLong(&vnew) > 0) {
    pval = vnew;
  } else {
    Serial.print(F("# (not updated)\r\n"));
  }
}

void ParamSettings::manualReadParam(const char *desc, unsigned long &pval)
{
  long vnew;
  Serial.print("# Enter ");
  Serial.print(desc);
  Serial.print("(");
  Serial.print(pval);
  Serial.print("): ");
  if ((Supervisor::blockingReadLong(&vnew) > 0) && (vnew >= 0)) {
    pval = vnew;
  } else {
    Serial.print(F("# (not updated)\r\n"));
  }
}

void ParamSettings::manualReadParam(const char *desc, unsigned int &pval)
{
  long vnew;
  Serial.print("# Enter ");
  Serial.print(desc);
  Serial.print("(");
  Serial.print(pval);
  Serial.print("): ");
  if ((Supervisor::blockingReadLong(&vnew) > 0) && (vnew >= 0)) {
    pval = vnew;
  } else {
    Serial.print(F("# (not updated)\r\n"));
  }
}

void ParamSettings::manualReadPercent(const char *desc, uint8_t &pval)
{
  long pnew;
  Serial.print("# Enter ");
  Serial.print(desc);
  Serial.print("(");
  Serial.print(pval);
  Serial.print("%): ");
  if ((Supervisor::blockingReadLong(&pnew) > 0) && (pnew >= 0) && (pnew <= 100)) {
    pval = pnew;
  } else {
    Serial.print(F("# (not updated)\r\n"));
  }

}


void ParamSettings::manualReadPump(const char *desc, uint8_t &pval)
{
  uint8_t pnew;
  Serial.print("# Enter ");
  Serial.print(desc);
  Serial.print("(");
  Serial.print('A' + pval);
  Serial.print("): ");

  if (Supervisor::blockingReadPump(&pnew) > 0) {
    pval = pnew;
  } else {
    Serial.print(F("# (not updated)\r\n"));
  }
}

void ParamSettings::serialWriteParams(void)
{ 
  Serial.print(F("# Current settings:\r\n"));
  formatParams(Supervisor::outbuf, Supervisor::outbufLen);
  Serial.write(Supervisor::outbuf);
}


