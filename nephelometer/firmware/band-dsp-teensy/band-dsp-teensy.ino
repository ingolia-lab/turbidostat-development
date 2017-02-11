#include <ADC.h>
#include <EEPROM.h>
#include <SPI.h>

const int irLedPin = 2;

const int pgaCSPin = 15;
const int pgaSCKPin = 13;
const int pgaMOSIPin = 11;

const int motor1Pin = 16;
const int motor2Pin = 17;

// MODE3 = 1,1 is better so device select pin doesn't clock
SPISettings pgaSettings(4000000 /* 4 MHz */, MSBFIRST, SPI_MODE3);

// const int motorPin = XXX
// const int adcChipSelPin = XXX

#define MEASURE_PGA 0x03

#define MEASURE_HALF_CYCLE_USEC 52
#define MEASURE_ADC_DELAY_USEC 25

#define MEASURE_NEQUIL 16
#define MEASURE_NMEASURE 4096
#define MEASURE_USEC (((long) MEASURE_NMEASURE) * 2 * MEASURE_HALFT_USEC)
#define MEASURE_INTERVAL_USEC 500000

struct measure_conf_struct {
  uint8_t pga;
  unsigned long halfCycleUsec;
  unsigned long adcDelayUsec;
  unsigned int nEquil;
  unsigned int nMeasure;
};

/* A useful scratch buffer to format output e.g. with snprintf() */
const int outbuf_len = 256;
char outbuf[outbuf_len];

void manualAnnotate(void);
void manualMeasure(void);
void manualPump(void);

void delayScan(uint8_t pga, unsigned int nEquil, unsigned int nMeasure, unsigned long maxAdcDelay);
int measure(const struct measure_conf_struct *conf, long *ttlon, long *ttloff);

void serialPrintMicros(unsigned long t);
inline unsigned long delayIfNeeded(unsigned long until);
void setPGA(uint8_t setting);

int blockingReadLong(long *res);

ADC *adc = new ADC();

void setup() {
  pinMode(irLedPin, OUTPUT);
  digitalWrite(irLedPin, HIGH);

  pinMode(pgaCSPin, OUTPUT);
  digitalWrite(pgaCSPin, HIGH);

  pinMode(pgaSCKPin, OUTPUT);
  pinMode(pgaMOSIPin, OUTPUT);

  pinMode(motor1Pin, OUTPUT);
  digitalWrite(motor1Pin, LOW);
  pinMode(motor2Pin, OUTPUT);
  digitalWrite(motor2Pin, LOW);

  Serial.begin(9600);

  SPI.setSCK(pgaSCKPin);
  SPI.begin();

  pinMode(A10, INPUT);
  pinMode(A11, INPUT);

  adc->setAveraging(1);
  adc->setResolution(12);
  adc->setConversionSpeed(ADC_HIGH_SPEED);
  adc->setSamplingSpeed(ADC_VERY_HIGH_SPEED);

  setPGA(0x00);
}

void loop() {
//  unsigned long tStart = micros();

  manualLoop();

//  delayScan();

//  measureAndPrint(0x03);

//  delayIfNeeded(tStart + MEASURE_INTERVAL_USEC);
}

void manualLoop()
{
  Serial.print(F("# band-psd-teensy 17-02-10 setup [admp] > "));

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
      delayScan();
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

void manualMeasure(void)
{
  Serial.print(F("\r\n"));

  struct measure_conf_struct conf = { MEASURE_PGA, MEASURE_HALF_CYCLE_USEC, MEASURE_ADC_DELAY_USEC, MEASURE_NEQUIL, MEASURE_NMEASURE };
  
  while (1) {
    unsigned long tStart = micros();
    measureAndPrint(&conf);

    if (Serial.read() > 0) {
      break; 
    }

    delayIfNeeded(tStart + MEASURE_INTERVAL_USEC);
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

int measureAndPrint(const struct measure_conf_struct *conf)
{
  unsigned long tStart = micros();

  long ttlon, ttloff;
  
  int res = measure(conf, &ttlon, &ttloff);

  Serial.write("M\t");

  serialPrintMicros(tStart);
  
  Serial.write('\t');
  if (res < 0) {
    Serial.write("***");
    Serial.print(res);
  } else {
    long avg10 = (10 * (ttloff - ttlon)) / conf->nMeasure;
    Serial.print(avg10);
  }
  Serial.println();

  return res;
}

/* Make a phase-sensitive scattered light measurement
 * Using timing and gain parameters in `conf`, measure scattered light.
 * Store the measured values with LED on in *ttlon and with LED off in *ttloff and return 0
 * A real signal should give *ttloff > *ttlon
 * conf->nMeasure measurements are added, each one is [0,4095] so *ttloff, *ttlon is in [0,4095*conf->nMeasure]
 * In the event of a timing failure, *ttloff and *ttlon are unreliable and a negative value is returned
 */
int measure(const struct measure_conf_struct *conf, long *ttlon, long *ttloff)
{
  *ttlon = 0;
  *ttloff = 0;
  
  setPGA(conf->pga);

  unsigned long tStart = micros();
  
  for (unsigned int i = 0; i < conf->nEquil; i++) {
    digitalWrite(irLedPin, LOW);
    if (!delayIfNeeded(tStart + (1 + 2 * i) * conf->halfCycleUsec)) {
      return -1;
    }
    digitalWrite(irLedPin, HIGH);
    if (!delayIfNeeded(tStart + (2 + 2 * i) * conf->halfCycleUsec)) {
      return -2;
    }
  }

  unsigned long tMeasure = tStart + 2 * conf->nEquil * conf->halfCycleUsec;

  for (unsigned int i = 0; i < conf->nMeasure; i++) {
    unsigned long tCycle = tMeasure + 2 * i * conf->halfCycleUsec;
    digitalWrite(irLedPin, LOW);
    
    if (!delayIfNeeded(tCycle + conf->adcDelayUsec)) {
      return -3;
    }
    *ttlon += adc->analogRead(A10);

    if (!delayIfNeeded(tCycle + conf->halfCycleUsec)) {
      return -4;
    }
    digitalWrite(irLedPin, HIGH);

    if (!delayIfNeeded(tCycle + conf->halfCycleUsec + conf->adcDelayUsec)) {
      return -5;
    }
    *ttloff += adc->analogRead(A10);

    if (!delayIfNeeded(tCycle + 2 * conf->halfCycleUsec)) {
      return -6;
    }
  }

  return 0;
}

#define DELAY_SCAN_PGA                 0x03
#define DELAY_SCAN_NEQUIL              MEASURE_NEQUIL
#define DELAY_SCAN_NMEASURE            256
#define DELAY_SCAN_ADC_USEC            11
#define DELAY_SCAN_MIN_HALF_CYCLE_USEC 40
#define DELAY_SCAN_MAX_HALF_CYCLE_USEC 60

void delayScan(void)
{
  struct measure_conf_struct conf = { DELAY_SCAN_PGA, 0, 0, DELAY_SCAN_NEQUIL, DELAY_SCAN_NMEASURE };

  for (unsigned long halfCycle = DELAY_SCAN_MIN_HALF_CYCLE_USEC; halfCycle <= DELAY_SCAN_MAX_HALF_CYCLE_USEC; halfCycle++) {
    conf.halfCycleUsec = halfCycle;
    for (unsigned long adcDelay = 0; adcDelay <= halfCycle - DELAY_SCAN_ADC_USEC; adcDelay++) {
      conf.adcDelayUsec = adcDelay;
      
      long ttlon, ttloff;

      int res = measure(&conf, &ttlon, &ttloff);

      if (res < 0) {
        Serial.write("# ");
      }

      Serial.write('\t');
      Serial.print(halfCycle);

      Serial.write('\t');
      Serial.print(adcDelay);

      if (res < 0) {
        Serial.write("\tERR ");
        Serial.print(res);
      } else {
        long diff10 = 10 * (ttloff - ttlon) / ((long) conf.nMeasure);
    
        Serial.write('\t');
        Serial.print(diff10);
      }
      
      Serial.println();
    }
  }
}

/* Delay until a specified time in microseconds
 * Return the number of microseconds delayed
 * If the current time is at or after `until` do not delay and return 0.
 */
inline unsigned long delayIfNeeded(unsigned long until)
{
  unsigned long now = micros();
  if (now < until) {
    delayMicroseconds(until - now);
    return until - now;
  } else {
    return 0;
  }
}

/* Set the gain on the programmable gain amplifier (PGA)
 * Use SPI to set the gain
 */
void setPGA(uint8_t setting)
{
  SPI.beginTransaction(pgaSettings);
  digitalWrite(pgaCSPin, LOW);
  SPI.transfer(0x40);
  SPI.transfer(setting);
  digitalWrite(pgaCSPin, HIGH);
  SPI.endTransaction();
}

#define PGA_NSETTING 8
long pgaScales[PGA_NSETTING] = { 1, 2, 4, 5, 8, 10, 16, 32 };

/* Return the scaling factor for a PGA setting
 * For undefined settings, return -1 instead
 */
long pgaScale(uint8_t setting)
{
  return (setting < PGA_NSETTING) ? pgaScales[setting] : -1;
}

/* Print a microseconds time to Serial
 * Print the number of seconds and (as a decimal) milliseconds in a microseconds time t
 */
void serialPrintMicros(unsigned long t)
{
  Serial.print(t / ((unsigned long) 1000000));
  Serial.write('.');
  Serial.print(t / ((unsigned long)  100000) % 10);
  Serial.print(t / ((unsigned long)   10000) % 10);
  Serial.print(t / ((unsigned long)    1000) % 10);
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


