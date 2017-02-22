#include <EEPROM.h>

#include "controller.h"
#include "supervisor.h"

void Controller::writeEepromLong(unsigned int base, unsigned int slot, long value)
{
  base = base + (slot * sizeof(long));
  const byte *b = (const byte *) (const void *) &value;
  for (unsigned int i = 0; i < sizeof(long); i++) {
    EEPROM.write(base + i, b[i]);
  } 
}

long Controller::readEepromLong(unsigned int base, unsigned int slot)
{
  base = base + (slot * sizeof(long));
  long x;
  byte *b = (byte *) (void *) &x;   
  for (unsigned int i = 0; i < sizeof(long); i++) {
    b[i] = EEPROM.read(base + i);
  }
  return x;
}

void Controller::manualReadParam(const char *desc, long &pval)
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

