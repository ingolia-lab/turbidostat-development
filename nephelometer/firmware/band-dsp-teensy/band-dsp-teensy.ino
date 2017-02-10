#include <ADC.h>
#include <EEPROM.h>
#include <SPI.h>

const int irLedPin = 2;

const int pgaCSPin = 15;
const int pgaSCKPin = 13;
const int pgaMOSIPin = 11;

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

void manualMeasure(void);

void delayScan(uint8_t pga, unsigned int nEquil, unsigned int nMeasure, unsigned long maxAdcDelay);
int measure(const struct measure_conf_struct *conf, long *ttlon, long *ttloff);

void serialPrintMicros(unsigned long t);
inline unsigned long delayIfNeeded(unsigned long until);
void setPGA(uint8_t setting);

ADC *adc = new ADC();

void setup() {
  pinMode(irLedPin, OUTPUT);
  digitalWrite(irLedPin, HIGH);

  pinMode(pgaCSPin, OUTPUT);
  digitalWrite(pgaCSPin, HIGH);

  pinMode(pgaSCKPin, OUTPUT);
  pinMode(pgaMOSIPin, OUTPUT);

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
  Serial.print(F("# band-psd-teensy 17-02-10 setup [adm] > "));

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
      unsigned long tStart = micros();

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

long pgaScale(uint8_t setting)
{
  return (setting < PGA_NSETTING) ? pgaScales[setting] : -1;
}

void serialPrintMicros(unsigned long t)
{
  Serial.print(t / ((unsigned long) 1000000));
  Serial.write('.');
  Serial.print(t / ((unsigned long)  100000) % 10);
  Serial.print(t / ((unsigned long)   10000) % 10);
  Serial.print(t / ((unsigned long)    1000) % 10);
}

