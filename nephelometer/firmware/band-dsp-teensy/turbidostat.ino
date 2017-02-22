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
 
void TurbidoParams::readEeprom(void)
{
  unsigned int e = 0;
  byte *b = (byte *) (void *) &pumpOn; 
  for (unsigned int i = 0; i < sizeof(pumpOn); e++, i++) {
    b[i] = EEPROM.read(eepromStart + e);
  } 

  b = (byte *) (void *) &pumpOff; 
  for (unsigned int i = 0; i < sizeof(pumpOff); e++, i++) {
    b[i] = EEPROM.read(eepromStart + i);
  } 
}

void TurbidoParams::writeEeprom(void)
{
  unsigned int e = 0;

  const byte *b = (const byte *) (const void *) &pumpOn;
  for (unsigned int i = 0; i < sizeof(pumpOn); e++, i++) {
    EEPROM.write(eepromStart + e, b[i]);
  } 

  b = (const byte *) (const void *) &pumpOff;
  for (unsigned int i = 0; i < sizeof(pumpOff); e++, i++) {
    EEPROM.write(eepromStart + e, b[i]);
  }
}

void TurbidoParams::format(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# Pump on @ %ld\r\n# Pump off @ %ld\r\n", 
           pumpOn, pumpOff);
}

