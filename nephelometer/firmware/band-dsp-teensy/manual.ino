#include "controller.h"
#include "manual.h"
#include "supervisor.h"

ManualController::ManualController(Supervisor &s)
{
  _nCommands = 9;
  _commands = new ManualCommand*[_nCommands];
  _commands[0] = new ManualAnnotate(s);
  _commands[1] = new ManualStartController(s);
  _commands[2] = new ManualDelayScan(s);
  _commands[3] = new ManualHelp(s, *this);
  _commands[4] = new ManualMeasure(s);
  _commands[5] = new ManualNephelSettings(s);
  _commands[6] = new ManualPump(s);
  _commands[7] = new ManualSetup(s);
  _commands[8] = new ManualTestNephel(s);

  _commandChars = new char[_nCommands + 1];
  for (unsigned int i = 0; i < _nCommands; i++) {
    _commandChars[i] = _commands[i]->letter();
  }
  _commandChars[_nCommands] = 0;

  Serial.println("# ManualController initialized");
}

int ManualController::loop(void)
{
  snprintf(Supervisor::outbuf, Supervisor::outbufLen, "# %s manual [%s] > ", _name, _commandChars);
  Serial.write(Supervisor::outbuf);

  int cmd;  
  while ((cmd = Serial.read()) < 0) {
    delay(1);
  }
  
  Serial.write(cmd);

  for (unsigned int i = 0; i < _nCommands; i++) {
    if (cmd == _commands[i]->letter()) {
      _commands[i]->run();
      return 0;
    }
  }
  
  Serial.print(F("\r\n# Unknown command: "));
  Serial.write((char) cmd);
  Serial.println();

  return 0;
}

void ManualController::serialWriteCommands(void)
{
  Serial.println("# COMMANDS:");
  for (unsigned int i = 0; i < _nCommands; i++) {
    snprintf(Supervisor::outbuf, Supervisor::outbufLen, "#   %c %22s   %s\r\n", _commands[i]->letter(), _commands[i]->name(), _commands[i]->help());
    Serial.write(Supervisor::outbuf);
  }
}

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

void ManualStartController::run(void)
{
  Serial.println();
  Serial.println(F("# Start a manual controller:"));
  supervisor().pickNextController();
}

void ManualDelayScan::run(void)
{
  supervisor().nephelometer().delayScan();
}

void ManualHelp::run(void) 
{
  Serial.println();
  _manualCtrl.serialWriteCommands();
  supervisor().serialWriteControllers();  
}

void ManualMeasure::run(void)
{
  Serial.println();

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
    if (pno > 1) {
      Serial.print(",");
    }
    Serial.print(pno);
  }
  Serial.print(F("]: "));

  int ch;
  while ((ch = Serial.read()) < 0) {
    delay(1);
  }
  Serial.write(ch);
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

void ManualSetup::run(void)
{
  Serial.println(F("\r\n# Manually configure a controller"));
  supervisor().manualSetupController();
}

void ManualTestNephel::run(void)
{
  Serial.println(F("\r\n# Switching to test nephelometer!"));
  supervisor().useTestNephel();
}

