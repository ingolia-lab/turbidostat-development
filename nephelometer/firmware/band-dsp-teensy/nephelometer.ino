#include <SPI.h>

#include "nephelometer.h"
#include "util.h"

// struct NephelHW defaultNephelHW = { irLedPin, pgaCSPin, pgaSCKPin, pgaMOSIPin, adcSignalPin, adcRefPin };

static const uint8_t defaultPga = 0x03;
static const long defaultHalfCycleUsec = 52;
static const long defaultAdcDelayUsec = 25;
static const int defaultNEquil = 16;
static const int defaultNMeasure = 4096;

NephelMeasure::NephelMeasure():
  pga(defaultPga),
  halfCycleUsec(defaultHalfCycleUsec),
  adcDelayUsec(defaultAdcDelayUsec),
  nEquil(defaultNEquil),
  nMeasure(defaultNMeasure)
{
}

long NephelMeasure::pgaScale(void) { return Nephel::pgaScale(pga); }

const long Nephel::pgaScales[] = { 1, 2, 4, 5, 8, 10, 16, 32 };

// SPI MODE3 = 1,1 is better so device select pin doesn't clock

Nephel::Nephel(int irLedPin, int pgaCSPin, int pgaSCKPin, int pgaMOSIPin, int adcSignalPin , int adcRefPin):
  _pgaSPISettings(4000000 /* 4 MHz */, MSBFIRST, SPI_MODE3),
  _adc()
{ 
  pinMode(_irLedPin, OUTPUT);
  digitalWrite(_irLedPin, HIGH);

  pinMode(_pgaCSPin, OUTPUT);
  digitalWrite(_pgaCSPin, HIGH);

  pinMode(_pgaSCKPin, OUTPUT);
  pinMode(_pgaMOSIPin, OUTPUT);

  pinMode(_adcSignalPin, INPUT);
  pinMode(_adcRefPin, INPUT);

  SPI.setSCK(_pgaSCKPin);

  _adc.setAveraging(1);
  _adc.setResolution(12);
  _adc.setConversionSpeed(ADC_HIGH_SPEED);
  _adc.setSamplingSpeed(ADC_VERY_HIGH_SPEED);

  setPga(0x00);
}                

/* Set the gain on the programmable gain amplifier (PGA)
 * Use SPI to set the gain
 */
int Nephel::setPga(uint8_t setting)
{
  if (setting < Nephel::nPgaScales) {
    SPI.beginTransaction(_pgaSPISettings);
    digitalWrite(_pgaCSPin, LOW);
    SPI.transfer(0x40);
    SPI.transfer(setting);
    digitalWrite(_pgaCSPin, HIGH);
    SPI.endTransaction();

    return 0;
  } else {
    return -1;
  }
}

long Nephel::measure(const NephelMeasure &measure)
{
  long ttlon, ttloff;
  int res;
  
  if ((res = measurePeaks(measure, &ttlon, &ttloff))) {
    return -1;
  }

  return (((long) 10) * (ttloff - ttlon)) / measure.nMeasure;
}

/* Make a phase-sensitive scattered light measurement
 * Using timing and gain parameters in `conf`, measure scattered light.
 * Store the measured values with LED on in *ttlon and with LED off in *ttloff and return 0
 * A real signal should give *ttloff > *ttlon
 * conf->nMeasure measurements are added, each one is [0,4095] so *ttloff, *ttlon is in [0,4095*conf->nMeasure]
 * In the event of a timing failure, *ttloff and *ttlon are unreliable and a negative value is returned
 */
int Nephel::measurePeaks(const NephelMeasure &measure, long *ttlon, long *ttloff)
{
  *ttlon = 0;
  *ttloff = 0;

  setPga(measure.pga);
  
  unsigned long tStart = micros();
  
  for (unsigned int i = 0; i < measure.nEquil; i++) {
    digitalWrite(_irLedPin, LOW);
    if (!delayIfNeeded(tStart + (1 + 2 * i) * measure.halfCycleUsec)) {
      return -1;
    }
    digitalWrite(_irLedPin, HIGH);
    if (!delayIfNeeded(tStart + (2 + 2 * i) * measure.halfCycleUsec)) {
      return -2;
    }
  }

  unsigned long tMeasure = tStart + 2 * measure.nEquil * measure.halfCycleUsec;

  for (unsigned int i = 0; i < measure.nMeasure; i++) {
    unsigned long tCycle = tMeasure + 2 * i * measure.halfCycleUsec;
    digitalWrite(_irLedPin, LOW);
    
    if (!delayIfNeeded(tCycle + measure.adcDelayUsec)) {
      return -3;
    }
    *ttlon += _adc.analogRead(_adcSignalPin);

    if (!delayIfNeeded(tCycle + measure.halfCycleUsec)) {
      return -4;
    }
    digitalWrite(_irLedPin, HIGH);

    if (!delayIfNeeded(tCycle + measure.halfCycleUsec +measure.adcDelayUsec)) {
      return -5;
    }
    *ttloff += _adc.analogRead(_adcSignalPin);

    if (!delayIfNeeded(tCycle + 2 * measure.halfCycleUsec)) {
      return -6;
    }
  }

  return 0;
}

#define DELAY_SCAN_PGA                 0x03
#define DELAY_SCAN_NEQUIL              16
#define DELAY_SCAN_NMEASURE            256
#define DELAY_SCAN_ADC_USEC            11
#define DELAY_SCAN_MIN_HALF_CYCLE_USEC 40
#define DELAY_SCAN_MAX_HALF_CYCLE_USEC 60

void Nephel::delayScan()
{
  NephelMeasure conf;
  conf.pga = DELAY_SCAN_PGA;
  conf.halfCycleUsec = 0;
  conf.adcDelayUsec = 0;
  conf.nEquil = DELAY_SCAN_NEQUIL;
  conf.nMeasure = DELAY_SCAN_NMEASURE;

  for (unsigned long halfCycle = DELAY_SCAN_MIN_HALF_CYCLE_USEC; halfCycle <= DELAY_SCAN_MAX_HALF_CYCLE_USEC; halfCycle++) {
    conf.halfCycleUsec = halfCycle;
    for (unsigned long adcDelay = 0; adcDelay <= halfCycle - DELAY_SCAN_ADC_USEC; adcDelay++) {
      conf.adcDelayUsec = adcDelay;
      
      long ttlon, ttloff;

      int res = measurePeaks(conf, &ttlon, &ttloff);

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

