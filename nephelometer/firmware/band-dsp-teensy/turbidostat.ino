#include "turbidostat.h"
#include "util.h"

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

