#include "controller.h"
#include "manual.h"
#include "supervisor.h"

#define OUTBUF_LEN 512
const unsigned int Supervisor::outbufLen = OUTBUF_LEN;
char Supervisor::outbuf[OUTBUF_LEN] = "";

Supervisor::Supervisor(void):
  _neph(),
  _defaultController(ManualController(*this)),
  _runningController(&_defaultController),
  _nextController(&_defaultController)
{
  _nControllers = 2;
  _controllers = new Controller*[_nControllers];
  _controllers[0] = &_defaultController;
  _controllers[1] = new Turbido(_neph, _pumps[0]);
  Serial.println("# Supervisor initialized");
}

void Supervisor::begin(void)
{
  _runningController = &defaultController();
  _nextController    = _runningController;
  Serial.print("# Supervisor beginning, with controller ");
  Serial.println(_runningController->name());
}

void Supervisor::loop(void)
{
  if (_nextController != _runningController) {
    _runningController->end();
    if (_nextController->begin()) {
      Serial.println(F("# Problem switching to new controller -- entering default mode"));
      _runningController = &defaultController();
    } else {
      _runningController = _nextController;
    }
  }
  
  if (_runningController->loop()) {
    _nextController = &defaultController();
  }  
}

void Supervisor::serialWriteControllers(void)
{
  Serial.println("# CONTROLLERS:");
  for (unsigned int i = 0; i < _nControllers; i++) {
    snprintf(outbuf, outbufLen, "#   %c %25s\r\n", _controllers[i]->letter(), _controllers[i]->name());
    Serial.write(outbuf);
  }
}

Controller *Supervisor::pickController(void)
{
  serialWriteControllers();

  Serial.print("# Pick a controller: ");

  int ch;
  while ((ch = Serial.read()) < 0) {
    delay(1);
  }
  Serial.write(ch);
  
  for (unsigned int i = 0; i < _nControllers; i++) {
    if (_controllers[i]->letter() == ch) {
      Serial.print("=");
      Serial.println(_controllers[i]->name());
      return _controllers[i];
    }
  }

  Serial.print(" does not match known controller");
  return NULL;
}

void Supervisor::manualSetupController(void)
{
  Serial.print(F("# Pick a controller to configure\r\n"));
  Controller *c;
  if ((c = pickController()) != NULL) {
    c->manualSetParams();
  } else {
    Serial.print(F("# No controller picked to configure\r\n"));
  }
}

void Supervisor::pickNextController(void)
{  
  Serial.print(F("# Pick a controller to start\r\n"));
  Controller *c;
  if ((c = pickController()) != NULL) {
    _nextController = c;
  } else {
    Serial.print(F("# No controller picked\r\n"));
  }
}

void Supervisor::readEeprom(unsigned int eepromBase)
{
  long eepromVersion = readEepromLong(eepromBase, versionSlot);
  if (eepromVersion != version) {
    snprintf(outbuf, outbufLen, "# !!! Saved state version %ld does not match software version %ld\r\n", eepromVersion, version);
    Serial.print(outbuf);
    return;
  }

  int rc = (int) readEepromLong(eepromBase, runningControllerSlot);
  _runningController = (rc >= 0 && rc <= ((int) _nControllers)) ? (_controllers[rc]) : &_defaultController;

  NephelTiming nt = NephelTiming();
  nt.readEeprom(nephelBase);
  _neph.setTiming(nt);

  _runningController->readEeprom(controllerBase);
}

void Supervisor::writeEeprom(unsigned int eepromBase)
{
  writeEepromLong(eepromBase, versionSlot, version);

  long rcNo = 0;
  for (unsigned int i = 0; i < _nControllers; i++) {
    if (_runningController == _controllers[i]) {
      rcNo = (long) i;
    }
  }
  writeEepromLong(eepromBase, runningControllerSlot, rcNo);
  nephelometer().timing().writeEeprom(nephelBase);
  _runningController->writeEeprom(controllerBase);
}

void Supervisor::manualSetParams(void)
{
  Serial.println(F("# Supervisor: no manual parameters"));
}

void Supervisor::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Supervisor: no parameters\r\n");
}


/* Read a (long) integer from Serial
 * Read digits from serial until enter/return, store the result into *res, and return 1
 * If no digits are typed before enter/return, return 0 and leave *res unchanged
 * If a non-digit character is typed, return -1 immediately and leave *res unchanged
 */
int Supervisor::blockingReadLong(long *res)
{
  const int buflen = 12;
  char buffer[buflen];
  int bufpos = 0;

  int ch;
  
  do {
    ch = Serial.read();
    if (ch <= 0) {
       delay(1);
    } else if (ch == '\n' || ch == '\r') {
       Serial.println();
       break;
    } else if ((ch < '0' || ch > '9') && (bufpos != 0 || ch != '-')) {
       Serial.write('*');
       return -1;
    } else {
       buffer[bufpos] = (char) ch;
       Serial.write(ch);
       bufpos++;
       if (bufpos == (buflen - 1)) {
         break;
       } 
    }
  } while(1);
  
  if (bufpos > 0) {  
    buffer[bufpos] = '\0';
    *res = atol(buffer);
    return 1;
  } else {
    return 0;
  }
}


