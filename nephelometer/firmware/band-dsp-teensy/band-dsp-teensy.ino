#include <EEPROM.h>
#include <SPI.h>

#include "nephelometer.h"
#include "util.h"
 
static const int motor1Pin = 16;
static const int motor2Pin = 17;

/* A useful scratch buffer to format output e.g. with snprintf() */
const int outbuf_len = 256;
char outbuf[outbuf_len];

long tnext;
const long tstep = 1000;

int manualIdleAutoStart = 1;
long manualIdleTimeout = 90 * (long) 1000;

void manualAnnotate(void);
void manualMeasure(void);
void manualPump(void);

void serialPrintMicros(unsigned long t);

int blockingReadLong(long *res);

struct nephelometer_struct *neph;
struct nephel_measure_struct nephMeasure = measureDefault;

void setup() {
  neph = nephel_init(&hwDefault);
  
  pinMode(motor1Pin, OUTPUT);
  digitalWrite(motor1Pin, LOW);
  pinMode(motor2Pin, OUTPUT);
  digitalWrite(motor2Pin, LOW);

  Serial.begin(9600);
}

void loop() {
  manualLoop();
}

void manualLoop()
{
  Serial.print(F("# band-psd-teensy 17-02-10 setup [adgmp] > "));

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
      neph_delayScan(neph);
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
  Serial.write(pgaScale(nephMeasure.pga));
  Serial.print(F("x\r\n# NEW GAIN ["));

  for (uint8_t i = 0; i < nephel_pga_nsettings; i++) {
    if (i > 0) {
      Serial.print(", ");
    }
    Serial.print('a' + ((char) i));
    Serial.print("=");
    Serial.write(pgaScale(i));
    Serial.print("x");
  }

  Serial.print(F("]: "));

  int ch;
  while ((ch = Serial.read()) < 0) {
    delay(1);
  }
  if (ch >= 'a' && ch < ('a' + nephel_pga_nsettings)) {
    nephMeasure.pga = ch - 'a';
  } else {
    Serial.printf(F("\r\n# UNKNOWN SETTING, GAIN UNCHANGED"));
  }

  Serial.print(F("\r\n# CURRENT GAIN: "));
  Serial.write(pgaScale(nephMeasure.pga));
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
  int motorPin, ch;
  
  Serial.print(F("\r\n# Which pump [12]: "));
  while ((ch = Serial.read()) < 0) {
    delay(1);
  }
  if (ch == '1') {
    motorPin = motor1Pin;
  } else if (ch == '2') {
    motorPin = motor2Pin;
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
    digitalWrite(motorPin, HIGH);

    unsigned long tend = tstart + pumpDurationRequested * 1000;

    while (millis() < tend) {
      if (Serial.read() > 0) {
        break; 
      }
      delay(1);
    }

    unsigned long tstop = millis();
    digitalWrite(motorPin, LOW);

    unsigned long pumpDurationActual = tstop - tstart;

    snprintf(outbuf, outbuf_len, "\r\n# Pumped %ld.%03ld seconds\r\n", pumpDurationActual / 1000, pumpDurationActual % 1000);
    Serial.write(outbuf);  
  } else {
    Serial.print(F("\r\n# Manual pump cancelled\r\n"));
  }  
}

// Measurement

int measureAndPrint(void)
{
  unsigned long tStart = micros();

  long avg10;
  
  int res = nephel_measure(neph, &nephMeasure, &avg10);

  Serial.write("M\t");

  serialPrintMicros(tStart);
  
  Serial.write('\t');
  if (res < 0) {
    Serial.write("***");
    Serial.print(res);
  } else {
    Serial.print(avg10);
    Serial.write("\t");
    Serial.print(pgaScale(nephMeasure.pga));
  }
  Serial.println();

  return res;
}

/* Read a (long) integer from Serial
 * Read digits from serial until enter/return, store the result into *res, and return 1
 * If no digits are typed before enter/return, return 0 and leave *res unchanged
 * If a non-digit character is typed, return -1 immediately and leave *res unchanged
 */
int blockingReadLong(long *res)
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
    } else if (ch < '0' || ch > '9') {
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


