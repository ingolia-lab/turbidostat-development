#include "manual.h"
#include "supervisor.h"

void ManualAnnotate::run(void)
{
  int ch;
  
  Serial.print(F("\r\n# NOTE: "));
 
  while (1) {
    while ((ch = Serial.read()) < 0) {
      delay(1);
    }
    
    if (ch == '\n' || ch == '\r') {
      Serial.println();
      break; 
    }
    
    Serial.write(ch);
  } 
}

void ManualDelayScan::run(void)
{
  supervisor().nephelometer().delayScan();
}

void ManualHelp::run(void) 
{
  supervisor().help();  
}

void ManualMeasure::run(void)
{
  Serial.print(F("\r\n"));

  while (1) {
    unsigned long startMsec = millis();

    long avg10 = supervisor().nephelometer().measure();

    snprintf(Supervisor::outbuf, Supervisor::outbufLen, 
             "M\t%lu.%03lu\t%ld\t%ld",
             startMsec / ((unsigned long) 1000), startMsec % ((unsigned long) 1000),
             avg10, supervisor().nephelometer().pgaScale());
    Serial.println(Supervisor::outbuf);

    if (Serial.read() > 0) {
      break; 
    }

    Supervisor::delayIfNeeded(((long) 1000) * (startMsec + intervalMsec));
  }

  while (Serial.read() > 0) { /* Drain the buffer */ }
}

void ManualNephelSettings::run(void)
{  
  NephelTiming newTiming = supervisor().nephelometer().timing();
  newTiming.manualSetParams();
  supervisor().nephelometer().setTiming(newTiming);
// XXX WRITE TO EEPROM
}

void ManualPump::run(void)
{
  Serial.print(F("\r\n# Which pump ["));
  for (int pno = 1; pno <= supervisor().nPumps(); pno++) {
    Serial.print('1' + pno);
    Serial.print(",");
  }
  Serial.print(F("]: "));

  int ch;
  while ((ch = Serial.read()) < 0) {
    delay(1);
  }
  int pno = ch - '1';
  if (pno >= 0 && pno < supervisor().nPumps()) {
    Pump &p = supervisor().pump(pno);

    Serial.print(F("\r\n# Enter pump duration (sec): "));
    long pumpDurationRequested;
    if (Supervisor::blockingReadLong(&pumpDurationRequested) > 0) {
      Serial.print(F("# Planned pumping time: "));
      Serial.print(pumpDurationRequested);
      Serial.print(F(" sec (any key to interrupt)"));

      long totalOnBefore = p.totalOnMsec();
      unsigned long tstart = millis();
      unsigned long tend = tstart + pumpDurationRequested * 1000;
      p.setPumping(1);

      while (millis() < tend) {
        if (Serial.read() > 0) {
          break; 
        }
        delay(1);
      }

      p.setPumping(0);

      long pumpDurationActual = p.totalOnMsec() - totalOnBefore;

      snprintf(Supervisor::outbuf, Supervisor::outbufLen, 
               "\r\n# Pumped %ld.%03ld seconds\r\n", 
               pumpDurationActual / 1000, pumpDurationActual % 1000);
      Serial.write(Supervisor::outbuf);  
    } else {
      Serial.print(F("\r\n# Manual pump cancelled\r\n"));
    }
  } else {
    Serial.print(F("\r\n# Manual pump cancelled\r\n"));
  }
  

}

