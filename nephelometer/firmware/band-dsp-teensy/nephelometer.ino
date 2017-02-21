#include <SPI.h>

#include "nephelometer.h"
#include "util.h"

static const int irLedPin = 2;
static const int pgaCSPin = 15;
static const int pgaSCKPin = 13;
static const int pgaMOSIPin = 11;
static const int adcSignalPin = A10;
static const int adcRefPin = A11;

struct nephel_hw_struct hwDefault = { irLedPin, pgaCSPin, pgaSCKPin, pgaMOSIPin, adcSignalPin, adcRefPin };

struct nephelometer_struct {
  SPISettings *pgaSettings;
  ADC *adc;

  struct nephel_hw_struct hw;
};

#define MEASURE_PGA 0x03
#define MEASURE_HALF_CYCLE_USEC 52
#define MEASURE_ADC_DELAY_USEC 25
#define MEASURE_NEQUIL 16
#define MEASURE_NMEASURE 4096

struct nephel_measure_struct measureDefault = { MEASURE_PGA, MEASURE_HALF_CYCLE_USEC, MEASURE_ADC_DELAY_USEC, MEASURE_NEQUIL, MEASURE_NMEASURE };

const int nephel_pga_nsettings = 8;
static long pgaScales[nephel_pga_nsettings] = { 1, 2, 4, 5, 8, 10, 16, 32 };

static int setPGA(struct nephelometer_struct *nephel, uint8_t setting);
int measure_internal(struct nephelometer_struct *neph, const struct measure_conf_struct *measure, long *ttlon, long *ttloff);

struct nephelometer_struct *
nephel_init(const struct nephel_hw_struct *hw)
{
  struct nephelometer_struct *neph = new(struct nephelometer_struct);
  
  neph->hw = *hw;

  pinMode(neph->hw.irLedPin, OUTPUT);
  digitalWrite(neph->hw.irLedPin, HIGH);

  pinMode(neph->hw.pgaCSPin, OUTPUT);
  digitalWrite(neph->hw.pgaCSPin, HIGH);

  pinMode(neph->hw.pgaSCKPin, OUTPUT);
  pinMode(neph->hw.pgaMOSIPin, OUTPUT);

  pinMode(neph->hw.adcSignalPin, INPUT);
  pinMode(neph->hw.adcRefPin, INPUT);

  // MODE3 = 1,1 is better so device select pin doesn't clock
  neph->pgaSettings = new SPISettings(4000000 /* 4 MHz */, MSBFIRST, SPI_MODE3);

  SPI.setSCK(neph->hw.pgaSCKPin);
  SPI.begin();

  neph->adc = new ADC();

  neph->adc->setAveraging(1);
  neph->adc->setResolution(12);
  neph->adc->setConversionSpeed(ADC_HIGH_SPEED);
  neph->adc->setSamplingSpeed(ADC_VERY_HIGH_SPEED);

  setPGA(neph, 0x00);

  return neph;
}                

/* Set the gain on the programmable gain amplifier (PGA)
 * Use SPI to set the gain
 */
static int setPGA(struct nephelometer_struct *neph, uint8_t setting)
{
  if (setting < nephel_pga_nsettings) {
    SPI.beginTransaction(*(neph->pgaSettings));
    digitalWrite(neph->hw.pgaCSPin, LOW);
    SPI.transfer(0x40);
    SPI.transfer(setting);
    digitalWrite(neph->hw.pgaCSPin, HIGH);
    SPI.endTransaction();

    return 0;
  } else {
    return -1;
  }
}

/* Return the scaling factor for a PGA setting
 * For undefined settings, return -1 instead
 */
long pgaScale(uint8_t setting)
{
  return (setting < nephel_pga_nsettings) ? pgaScales[setting] : -1;
}

int nephel_measure(struct nephelometer_struct *neph, const struct nephel_measure_struct *measure, long *avg10)
{
  long ttlon, ttloff;
  int res;
  
  if ((res = measure_internal(neph, measure, &ttlon, &ttloff))) {
    return res;
  }

  *avg10 = (((long) 10) * (ttloff - ttlon)) / measure->nMeasure;

  return 0;
}

/* Make a phase-sensitive scattered light measurement
 * Using timing and gain parameters in `conf`, measure scattered light.
 * Store the measured values with LED on in *ttlon and with LED off in *ttloff and return 0
 * A real signal should give *ttloff > *ttlon
 * conf->nMeasure measurements are added, each one is [0,4095] so *ttloff, *ttlon is in [0,4095*conf->nMeasure]
 * In the event of a timing failure, *ttloff and *ttlon are unreliable and a negative value is returned
 */
int measure_internal(struct nephelometer_struct *neph, const struct nephel_measure_struct *measure, long *ttlon, long *ttloff)
{
  *ttlon = 0;
  *ttloff = 0;

  setPGA(neph, measure->pga);
  
  unsigned long tStart = micros();
  
  for (unsigned int i = 0; i < measure->nEquil; i++) {
    digitalWrite(neph->hw.irLedPin, LOW);
    if (!delayIfNeeded(tStart + (1 + 2 * i) * measure->halfCycleUsec)) {
      return -1;
    }
    digitalWrite(neph->hw.irLedPin, HIGH);
    if (!delayIfNeeded(tStart + (2 + 2 * i) * measure->halfCycleUsec)) {
      return -2;
    }
  }

  unsigned long tMeasure = tStart + 2 * measure->nEquil * measure->halfCycleUsec;

  for (unsigned int i = 0; i < measure->nMeasure; i++) {
    unsigned long tCycle = tMeasure + 2 * i * measure->halfCycleUsec;
    digitalWrite(neph->hw.irLedPin, LOW);
    
    if (!delayIfNeeded(tCycle + measure->adcDelayUsec)) {
      return -3;
    }
    *ttlon += neph->adc->analogRead(A10);

    if (!delayIfNeeded(tCycle + measure->halfCycleUsec)) {
      return -4;
    }
    digitalWrite(neph->hw.irLedPin, HIGH);

    if (!delayIfNeeded(tCycle + measure->halfCycleUsec +measure->adcDelayUsec)) {
      return -5;
    }
    *ttloff += neph->adc->analogRead(A10);

    if (!delayIfNeeded(tCycle + 2 * measure->halfCycleUsec)) {
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

void neph_delayScan(struct nephelometer_struct *neph)
{
  struct nephel_measure_struct conf = { DELAY_SCAN_PGA, 0, 0, DELAY_SCAN_NEQUIL, DELAY_SCAN_NMEASURE };

  for (unsigned long halfCycle = DELAY_SCAN_MIN_HALF_CYCLE_USEC; halfCycle <= DELAY_SCAN_MAX_HALF_CYCLE_USEC; halfCycle++) {
    conf.halfCycleUsec = halfCycle;
    for (unsigned long adcDelay = 0; adcDelay <= halfCycle - DELAY_SCAN_ADC_USEC; adcDelay++) {
      conf.adcDelayUsec = adcDelay;
      
      long ttlon, ttloff;

      int res = measure_internal(neph, &conf, &ttlon, &ttloff);

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

