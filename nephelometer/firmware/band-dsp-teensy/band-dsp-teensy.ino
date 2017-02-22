#include <EEPROM.h>
#include <SPI.h>

#include "nephelometer.h"
#include "pump.h"
#include "turbidostat.h"
#include "util.h"
 
static const int motor1Pin = 16;
static const int motor2Pin = 17;

/* A useful scratch buffer to format output e.g. with snprintf() */

int manualIdleAutoStart = 1;
long manualIdleTimeout = 90 * (long) 1000;

void manualAnnotate(void);
void manualMeasure(void);
void manualPump(void);

Nephel neph = Nephel();
NephelMeasure nephMeasure = NephelMeasure();

Pump pump1 = Pump(motor1Pin, 1);
Pump pump2 = Pump(motor2Pin, 1);

Turbido turbido = Turbido(neph, nephMeasure, pump1);

int turbidoRunning = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  if (turbidoRunning) {
    if (turbido.loop()) {
      turbidoRunning = 1;
    }
  } else {
    manualLoop();
  }
}

void manualLoop()
{
  Serial.print(F("# band-psd-teensy 17-02-10 setup [adgmprst] > "));

  int cmd;
  unsigned long idleStart = millis();
  
  while ((cmd = Serial.read()) < 0) {
    delay(1);
  }
  
  Serial.write(cmd);
  
  switch(cmd) {
    case 'a':
      manualAnnotate();
      break;

    case 'd':
      neph.delayScan();
      break;

    case 'g':
      manualGain();
      break;
      
    case 'm':
      manualMeasure();
      break;

    case 'p':
      manualPump();
      break;

    case 'r':
      pump1.reset();
      pump2.reset();
      break;

    case 's':
      turbido.manualSetParams();
      break;

    case 't':
      turbidoRunning = 1;
      Serial.println(F("\r\n# Turbidostat mode (q to quit)"));
      turbido.formatParams(outbuf, outbufLen);
      Serial.write(outbuf);
      break;

    default:
    {
      Serial.print(F("\r\n# Unknown command: "));
      Serial.write((char) cmd);
      Serial.println();
    }
    break;
  };
}

void manualAnnotate()
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

void manualGain(void)
{
  Serial.print(F("\r\n# CURRENT GAIN: "));
  Serial.write(nephMeasure.pgaScale());
  Serial.print(F("x\r\n# NEW GAIN ["));

  for (uint8_t i = 0; i < Nephel::nPgaScales; i++) {
    if (i > 0) {
      Serial.print(", ");
    }
    Serial.print('a' + ((char) i));
    Serial.print("=");
    Serial.write(Nephel::pgaScale(i));
    Serial.print("x");
  }

  Serial.print(F("]: "));

  int ch;
  while ((ch = Serial.read()) < 0) {
    delay(1);
  }
  if (ch >= 'a' && ch < ('a' + Nephel::nPgaScales)) {
    nephMeasure.pga = ch - 'a';
  } else {
    Serial.printf(F("\r\n# UNKNOWN SETTING, GAIN UNCHANGED"));
  }

  Serial.print(F("\r\n# CURRENT GAIN: "));
  Serial.write(nephMeasure.pgaScale());
  Serial.print(F("x\r\n"));
}

#define MANUAL_MEASURE_INTERVAL_USEC 500000

void manualMeasure(void)
{
  Serial.print(F("\r\n"));

  while (1) {
    unsigned long tStart = micros();
    measureAndPrint();

    if (Serial.read() > 0) {
      break; 
    }

    delayIfNeeded(tStart + MANUAL_MEASURE_INTERVAL_USEC);
  }
}

void manualPump(void)
{
  Pump *p;
  int ch;
  
  Serial.print(F("\r\n# Which pump [12]: "));
  while ((ch = Serial.read()) < 0) {
    delay(1);
  }
  if (ch == '1') {
    p = &pump1;
  } else if (ch == '2') {
    p = &pump2;
  } else {
    Serial.print(F("\r\n# Manual pump cancelled\r\n"));
    return; 
  }
  
  Serial.print(F("\r\n# Enter pump duration (sec): "));
  long pumpDurationRequested;
  if (blockingReadLong(&pumpDurationRequested) > 0) {
    Serial.print(F("# Planned pumping time: "));
    Serial.print(pumpDurationRequested);
    Serial.print(F(" sec (any key to interrupt)"));

    unsigned long tstart = millis();
    p->setPumping(1);

    unsigned long tend = tstart + pumpDurationRequested * 1000;

    while (millis() < tend) {
      if (Serial.read() > 0) {
        break; 
      }
      delay(1);
    }

    unsigned long tstop = millis();
    p->setPumping(0);

    unsigned long pumpDurationActual = tstop - tstart;

    snprintf(outbuf, outbufLen, "\r\n# Pumped %ld.%03ld seconds\r\n", pumpDurationActual / 1000, pumpDurationActual % 1000);
    Serial.write(outbuf);  
  } else {
    Serial.print(F("\r\n# Manual pump cancelled\r\n"));
  }  
}

// Measurement

void measureAndPrint(void)
{
  unsigned long tStart = micros();

  long avg10 = neph.measure(nephMeasure);

  Serial.write("M\t");

  formatMicros(outbuf, outbufLen, tStart);
  Serial.write(outbuf);
  
  Serial.write('\t');

  Serial.print(avg10);
  Serial.write("\t");
  Serial.print(nephMeasure.pgaScale());

  Serial.println();
}


