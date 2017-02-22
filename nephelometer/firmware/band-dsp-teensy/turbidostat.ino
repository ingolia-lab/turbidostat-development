#include "turbidostat.h"
#include "util.h"

Turbido::Turbido(Nephel &neph, const NephelMeasure &measure, Pump &pump):
  _neph(neph),
  _measure(measure),
  _pump(pump),
  _prevMsec(0)
{
  readEeprom();  
}

int Turbido::loop(void)
{
  long entryMsec = millis();

  if (entryMsec < _prevMsec + loopMsec) {
    delay(_prevMsec + loopMsec - entryMsec);
  }
  
  long startMsec = millis();
  long m = _neph.measure(_measure);

  if (_pump.isPumping() && m < _pumpOff) {
    _pump.setPumping(0);
  } else if ((!_pump.isPumping()) && m > _pumpOn) {
    _pump.setPumping(1);
  }

  long ptime = _pump.totalOnMsec();

  snprintf(outbuf, outbufLen, "%ld.%03ld\t%ld\t%ld.%03ld\r\n", 
           startMsec / ((long) 1000), startMsec % ((long) 100), m, 
           ptime / ((long) 1000), ptime % ((long) 1000));
  Serial.write(outbuf);

  _prevMsec = startMsec;

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

/*
extern const int turbidoEepromParamStart = 32;
extern const int turbidoEepromParamEnd = turbidoEepromParamStart + sizeof(struct turbido_param_struct);

int turbidoLoop(struct turbido_struct *turbido) {
  switch(turbido->state) {
    case turbidoStateStart:
    {
      turbido->state = turbidoStateRun;
      turbido->startMsec = millis();
      tnext = (millis() / tstep) + 2; // XXX
      Serial.println(F("\r\n# Turbidostat mode (q to quit)"));
      
      readEepromParams(&(turbido->params));
      formatParams(outbuf, outbuf_len, &(turbido->params));
      Serial.write(outbuf);
    }
    break;
    
    case turbidoStateRun:
    {
      turbidoLoop();
    }
    break;
    
    case turbidoStateStop:
    {
      turbido.state = STATE_MANUAL;
      turbido.pumpState = LOW;
      turbido.pumpLastTime = -1;
      digitalWrite(motorPin, turbido.pumpState);
    }
    break;

    default:
    {
      Serial.print(F("\r\n### UNKNOWN STATE\r\n"));
      delay(1000);
      turbido.state = STATE_MANUAL; 
    }
  };

}
 */

const int Turbido::eepromStart = 32;
const int Turbido::eepromEnd = eepromStart + sizeof(_pumpOn) + sizeof(_pumpOff);

void Turbido::readEeprom(void)
{
  unsigned int e = 0;
  byte *b = (byte *) (void *) &_pumpOn; 
  for (unsigned int i = 0; i < sizeof(_pumpOn); e++, i++) {
    b[i] = EEPROM.read(eepromStart + e);
  } 

  b = (byte *) (void *) &_pumpOff; 
  for (unsigned int i = 0; i < sizeof(_pumpOff); e++, i++) {
    b[i] = EEPROM.read(eepromStart + i);
  } 
}

void Turbido::writeEeprom(void)
{
  unsigned int e = 0;

  const byte *b = (const byte *) (const void *) &_pumpOn;
  for (unsigned int i = 0; i < sizeof(_pumpOn); e++, i++) {
    EEPROM.write(eepromStart + e, b[i]);
  } 

  b = (const byte *) (const void *) &_pumpOff;
  for (unsigned int i = 0; i < sizeof(_pumpOff); e++, i++) {
    EEPROM.write(eepromStart + e, b[i]);
  }
}

void Turbido::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Pump on @ %ld\r\n# Pump off @ %ld\r\n", 
           _pumpOn, _pumpOff);
}

void Turbido::manualSetParams(void)
{
  long v;
  
  Serial.print(F("\r\n# Current settings:\r\n"));
  formatParams(outbuf, outbufLen);
  Serial.write(outbuf);
  Serial.print(F("# Hit return to leave unchanged\r\n"));

  Serial.print(F("# Enter pump on (high) measurement ("));
  Serial.print(_pumpOn);
  Serial.print(F("): "));
  if (blockingReadLong(&v) > 0) {
    _pumpOn = v;
  } else {
    Serial.print(F("# (not updated)\r\n"));
  }
  
  Serial.print(F("# Enter pump off (low) measurement ("));
  Serial.print(_pumpOff);
  Serial.print(F("): "));
  if (blockingReadLong(&v) > 0) {
    _pumpOff = v;
  } else {
    Serial.print(F("# (not updated)\r\n"));
  }  
  
  Serial.print(F("# Writing new settings to EEPROM\r\n"));
  writeEeprom();

  Serial.print(F("# Current settings:\r\n"));
  formatParams(outbuf, outbufLen);
  Serial.write(outbuf);  
}

