#include "controller.h"
#include "manual.h"
#include "supervisor.h"

#define OUTBUF_LEN 512
const unsigned int Supervisor::outbufLen = OUTBUF_LEN;
char Supervisor::outbuf[OUTBUF_LEN] = "";

Supervisor::Supervisor(void):
  _neph()
{

  _nCommands = 2;
  _commands = new ManualCommand*[_nCommands];
  _commands[0] = new ManualAnnotate(*this);
  _commands[1] = new ManualDelayScan(*this);
  _commands[2] = new ManualHelp(*this);
  _commands[3] = new ManualMeasure(*this);
  _commands[4] = new ManualNephelSettings(*this);
  _commands[5] = new ManualPump(*this);

  _commandChars = new char[_nCommands + 1];
  for (unsigned int i = 0; i < _nCommands; i++) {
    _commandChars[i] = _commands[i]->letter();
  }
  _commandChars[_nCommands] = 0;

  _nControllers = 1;
  _controllers = new Controller*[_nControllers];
  _controllers[0] = new Turbido(_neph, _pumps[0]);
}

void Supervisor::begin(void)
{
  
}

void Supervisor::loop(void)
{
  snprintf(outbuf, outbufLen, "# %s setup [%s] > ", _version, _commandChars);
  Serial.write(outbuf);

  int cmd;  
  while ((cmd = Serial.read()) < 0) {
    delay(1);
  }
  
  Serial.write(cmd);

  for (unsigned int i = 0; i < _nCommands; i++) {
    if (cmd == _commands[i]->letter()) {
      _commands[i]->run();
      return;
    }
  }
  
  Serial.print(F("\r\n# Unknown command: "));
  Serial.write((char) cmd);
  Serial.println();
}

void Supervisor::help(void)
{
  snprintf(outbuf, outbufLen, "# %s help:\r\n", _version);
  Serial.write(outbuf);

  Serial.println("# COMMANDS:");
  for (unsigned int i = 0; i < _nCommands; i++) {
    snprintf(outbuf, outbufLen, "#   %c %25s %s\r\n", _commands[i]->letter(), _commands[i]->name(), _commands[i]->help());
    Serial.write(outbuf);
  }

  Serial.println("# CONTROLLERS:");
  for (unsigned int i = 0; i < _nControllers; i++) {
    snprintf(outbuf, outbufLen, "#   %c %25s\r\n", _controllers[i]->letter(), _controllers[i]->name());
    Serial.write(outbuf);
  }

  Serial.println("# END OF HELP");
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


