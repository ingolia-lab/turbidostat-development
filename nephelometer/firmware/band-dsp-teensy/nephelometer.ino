#include <SPI.h>

#include "nephelometer.h"
#include "supervisor.h"

NephelTiming::NephelTiming():
  pga(defaultPga),
  halfCycleUsec(defaultHalfCycleUsec),
  adcDelayUsec(defaultAdcDelayUsec),
  nEquil(defaultNEquil),
  nMeasure(defaultNMeasure)
{
}

long NephelTiming::pgaScale(void) { return Nephel::pgaScale(pga); }

void NephelTiming::readEeprom(unsigned int eepromStart)
{
  pga = readEepromLong(eepromStart, 0);
  halfCycleUsec = readEepromLong(eepromStart, 1);
  adcDelayUsec = readEepromLong(eepromStart, 2);
  nEquil = readEepromLong(eepromStart, 3);
  nMeasure = readEepromLong(eepromStart, 4);
}

void NephelTiming::writeEeprom(unsigned int eepromStart)
{
  writeEepromLong(eepromStart, 0, pga);
  writeEepromLong(eepromStart, 1, halfCycleUsec);
  writeEepromLong(eepromStart, 2, adcDelayUsec);
  writeEepromLong(eepromStart, 3, nEquil);
  writeEepromLong(eepromStart, 4, nMeasure);
}

void NephelTiming::formatParams(char *buf, unsigned int buflen)
{
  snprintf(buf, buflen, "# PGA %02x = %ldx\r\n# Half cycle %lu usec\r\n# ADC at %lu usec\r\n# Equilibrate %u cycles\r\n# Measure %u cycles\r\n", 
         pga, pgaScale(), halfCycleUsec, adcDelayUsec, nEquil, nMeasure);
}

void NephelTiming::manualSetParams(void)
{
  serialWriteParams();
  Serial.print(F("# Hit return to leave a parameter unchanged\r\n"));

  Serial.print(F("# PGA settings: "));
  for (uint8_t i = 0; i < Nephel::nPgaScales; i++) {
    if (i > 0) {
      Serial.print(", ");
    }
    Serial.write('0' + ((char) i));
    Serial.print("=");
    Serial.print(Nephel::pgaScale(i));
    Serial.print("x");
  }
  Serial.println();

  long pgaTmp = pga;
  manualReadParam("PGA setting", pgaTmp);
  pga = (pgaTmp > 0 && pgaTmp < Nephel::nPgaScales) ? ((uint8_t) pgaTmp) : pga;
  manualReadParam("Half cycle [usec]", halfCycleUsec);
  manualReadParam("ADC delay [usec]", adcDelayUsec);
  manualReadParam("Equilibrate [# cycles]", nEquil);
  manualReadParam("Measure [# cycles]", nMeasure);
  
  serialWriteParams();
}

const long Nephel::pgaScales[] = { 1, 2, 4, 5, 8, 10, 16, 32 };

// SPI MODE3 = 1,1 is better so device select pin doesn't clock

Nephel::Nephel(int irLedPin, int pgaCSPin, int pgaSCKPin, int pgaMOSIPin, int adcSignalPin , int adcRefPin):
  _irLedPin(irLedPin),
  _pgaCSPin(pgaCSPin),
  _pgaMOSIPin(pgaMOSIPin),
  _adcSignalPin(adcSignalPin),
  _adcRefPin(adcRefPin),
  _pgaSPISettings(4000000 /* 4 MHz */, MSBFIRST, SPI_MODE3),
  _adc()
{
  pinMode(_irLedPin, OUTPUT);
  digitalWrite(_irLedPin, HIGH);

  pinMode(_pgaCSPin, OUTPUT);
  digitalWrite(_pgaCSPin, HIGH);

  pinMode(_adcSignalPin, INPUT);
  pinMode(_adcRefPin, INPUT);

  _adc.setAveraging(1);
  _adc.setResolution(12);
  _adc.setConversionSpeed(ADC_HIGH_SPEED);
  _adc.setSamplingSpeed(ADC_VERY_HIGH_SPEED);

  Serial.println("# Nephelometer initialized");
}                

/* Set the gain on the programmable gain amplifier (PGA)
 * Use SPI to set the gain
 */
int Nephel::setPga(uint8_t setting)
{
  if (setting < nPgaScales) {
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

long Nephel::measure(void)
{
  long ttlon, ttloff;
  int res;
  
  if ((res = measurePeaks(_timing, &ttlon, &ttloff))) {
    Serial.print("# !!! Nephelometer measurePeaks failed");
    Serial.println(res);
    return 0;
  }

  return (((long) 10) * (ttloff - ttlon)) / ((long) _timing.nMeasure);
}

/* Make a phase-sensitive scattered light measurement
 * Using timing and gain parameters in `conf`, measure scattered light.
 * Store the measured values with LED on in *ttlon and with LED off in *ttloff and return 0
 * A real signal should give *ttloff > *ttlon
 * conf->nMeasure measurements are added, each one is [0,4095] so *ttloff, *ttlon is in [0,4095*conf->nMeasure]
 * In the event of a timing failure, *ttloff and *ttlon are unreliable and a negative value is returned
 */
int Nephel::measurePeaks(const NephelTiming &timing, long *ttlon, long *ttloff)
{
  *ttlon = 0;
  *ttloff = 0;

  if (setPga(timing.pga) < 0) {
    return -10;
  }
  
  unsigned long tStart = micros();
  
  for (unsigned int i = 0; i < timing.nEquil; i++) {
    digitalWrite(_irLedPin, LOW);
    if (!Supervisor::delayIfNeeded(tStart + (1 + 2 * i) * timing.halfCycleUsec)) {
      return -1;
    }
    digitalWrite(_irLedPin, HIGH);
    if (!Supervisor::delayIfNeeded(tStart + (2 + 2 * i) * timing.halfCycleUsec)) {
      return -2;
    }
  }

  unsigned long tMeasure = tStart + 2 * timing.nEquil * timing.halfCycleUsec;

  for (unsigned int i = 0; i < timing.nMeasure; i++) {
    unsigned long tCycle = tMeasure + 2 * i * timing.halfCycleUsec;
    digitalWrite(_irLedPin, LOW);
    
    if (!Supervisor::delayIfNeeded(tCycle + timing.adcDelayUsec)) {
      return -3;
    }
    *ttlon += _adc.analogRead(_adcSignalPin);

    if (!Supervisor::delayIfNeeded(tCycle + timing.halfCycleUsec)) {
      return -4;
    }
    digitalWrite(_irLedPin, HIGH);

    if (!Supervisor::delayIfNeeded(tCycle + timing.halfCycleUsec +timing.adcDelayUsec)) {
      return -5;
    }
    *ttloff += _adc.analogRead(_adcSignalPin);

    if (!Supervisor::delayIfNeeded(tCycle + 2 * timing.halfCycleUsec)) {
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
  NephelTiming conf;
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
      } else {
        Serial.write("D\t");
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

TestNephel::TestNephel(const Pump &goodPump, const Pump &badPump, unsigned long turbidity, unsigned long goodness, unsigned long doubleSeconds, unsigned long fillSeconds):
  _doubleSeconds(doubleSeconds),
  _fillSeconds(fillSeconds),
  _goodPump(goodPump),
  _badPump(badPump),
  _turbidity(turbidity),
  _goodness(goodness),
  _lastUpdateMsec(millis()),
  _lastUpdateGoodMsec(goodPump.totalOnMsec()),
  _lastUpdateBadMsec(badPump.totalOnMsec())
{
  Serial.println(F("# TestNephel() initialized"));
}

long TestNephel::measure(void)
{
  update();

  long rawMeasure = ((long) _turbidity) * pgaScale() / _measureFactor;
  rawMeasure += nephelNoise();
  return (rawMeasure > _maxMeasure) ? _maxMeasure : rawMeasure;
}

long TestNephel::nephelNoise(void)
{
  uint16_t x = random();
  return -5 + ((x & 0x0001) ? 1 : 0) + ((x & 0x0002) ? 1 : 0)
    + ((x & 0x0004) ? 1 : 0) + ((x & 0x0008) ? 1 : 0)
    + ((x & 0x0010) ? 1 : 0) + ((x & 0x0020) ? 1 : 0)
    + ((x & 0x0040) ? 1 : 0) + ((x & 0x0080) ? 1 : 0)
    + ((x & 0x0100) ? 1 : 0) + ((x & 0x0200) ? 1 : 0)
    + ((x & 0x0400) ? 1 : 0);
}
    
void TestNephel::update(void)
{
  unsigned long nowMsec = millis(), goodMsec = _goodPump.totalOnMsec(), badMsec = _badPump.totalOnMsec();

  unsigned long dtMsec = nowMsec - _lastUpdateMsec;
  _lastUpdateMsec = nowMsec;

  unsigned long dgoodMsec = goodMsec - _lastUpdateGoodMsec;
  _lastUpdateGoodMsec = goodMsec;

  unsigned long dbadMsec = badMsec - _lastUpdateBadMsec;
  _lastUpdateBadMsec = badMsec;

  unsigned long dpumpMsec = dgoodMsec + dbadMsec;

  // tdouble ~ 1k to 10k, dt ~1k, gf1M ~ 70 - 700
  // N.B. 693 ~ 1000 log(2)
  unsigned long growthFactor1M = (dtMsec * ((unsigned long) 693)) / doubleSeconds();
  unsigned long goodnessFactor1k = growthGoodness1k(_goodness);
  unsigned long goodGrowthFactor1M = growthFactor1M * goodnessFactor1k / 1000;
  unsigned long dg = goodGrowthFactor1M * _turbidity / 1000000;

  unsigned long dilutionFactor1M = (dpumpMsec * 1000) / fillSeconds();
  unsigned long df = dilutionFactor1M * _turbidity / 1000000;

  unsigned long newTurbidity = _turbidity + dg - df;

  snprintf(Supervisor::outbuf, Supervisor::outbufLen, 
           "# Test: T(0) = %lu, dt = %lu.%03lu, dpump = %lu.%03lu, gf = %lu.%06lu, gg = %lu.%03lu, ggf = %lu.%06lu, df = %lu.%06lu, dg = %lu, df = %lu, T(f) = %lu",
           _turbidity, dtMsec/1000, dtMsec%1000, dpumpMsec/1000, dpumpMsec%1000, 
           growthFactor1M/1000000, growthFactor1M%1000000,
           goodnessFactor1k/1000, goodnessFactor1k%1000,
           goodGrowthFactor1M/1000000, goodGrowthFactor1M%1000000,
           dilutionFactor1M/1000000, dilutionFactor1M%1000000,
           dg, df, newTurbidity);
  Serial.println(Supervisor::outbuf);

  unsigned long goodAdded100k = (dgoodMsec * 100) / fillSeconds();
  unsigned long totalAdded100k = ((dgoodMsec + dbadMsec) * 100) / fillSeconds();

  unsigned long newGoodness = (_goodness * 100000 + 10000 * goodAdded100k) / (100000 + totalAdded100k);

  snprintf(Supervisor::outbuf, Supervisor::outbufLen, 
           "# Test: G(0) = %lu, g+ = %lu.%05lu, t+ = %lu.%05lu, G(f) = %lu",
           _goodness, goodAdded100k/100000, goodAdded100k%100000, 
           totalAdded100k/100000, totalAdded100k%100000, newGoodness);
  Serial.println(Supervisor::outbuf);

  _turbidity = (newTurbidity > _maxTurbidity) ? _maxTurbidity : newTurbidity;
  _goodness = (newGoodness > _maxGoodness) ? _maxGoodness : newGoodness;
}

unsigned long TestNephel::growthGoodness1k(unsigned long goodness)
{
  return (_goodnessVmax1k * goodness) / (goodness + _goodnessKM);
}

void TestNephel::delayScan(void)
{
  Serial.println(F("# Test Nephelometer stub -- no delayScan()"));
}


