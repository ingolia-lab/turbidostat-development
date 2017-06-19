#ifndef _nephelometer_h
#define _nephelometer_h 1

/*
 * Control the nephelometer (turbidity measurement) circuits
 */

#include <SPI.h>

#include "pump.h"
#include "settings.h"

/*
 * Timing and other software-configured settings for turbidity measurements
 */
class NephelTiming : public ParamSettings {
public:
  // Create a new timing parameter set with compiled default values
  NephelTiming();

  void readEeprom(unsigned int eepromBase);
  void writeEeprom(unsigned int eepromBase);
  void formatParams(char *buf, unsigned int buflen);
  void manualSetParams(void);

  // Setting byte for the programmable-gain amplifier
  uint8_t pga;
  // Microseconds per half cycle of LED blinking
  unsigned long halfCycleUsec;
  // Delay from LED 
  unsigned long adcDelayUsec;
  
  unsigned int nEquil;
  unsigned int nMeasure;

  long pgaScale(void);

  static const uint8_t defaultPga = 0x03;
  static const long defaultHalfCycleUsec = 52;
  static const long defaultAdcDelayUsec = 25;
  static const int defaultNEquil = 16;
  static const int defaultNMeasure = 4096;
};

class Nephel
{
  public:
    Nephel(int irLedPin = 17, int pgaCSPin = 5, int adcCSPin = 6, int sckPin = 24, int mosiPin = 23, int misoPin = 22, uint8_t pgaSetting = 0x03);
    virtual long measure();

    static const int nPgaScales = 8;
    static const long pgaScales[];

    static long pgaScale(uint8_t setting) { return (setting < nPgaScales) ? pgaScales[setting] : -1; }
    long pgaScale(void) { return pgaScale(_pgaSetting); }
  private:
    int _irLedPin;
    int _pgaCSPin;
    int _adcCSPin;
    int _sckPin;
    int _mosiPin;
    int _misoPin;
    uint8_t _pgaSetting;

    SPISettings _pgaSPISettings;
    SPISettings _adcSPISettings;

    int setPga(uint8_t setting);

    static const int nMeasure = 1024;
    static const int nEquil = 16;

    static const unsigned long usecAdcOn = 25;
    static const unsigned long usecSpiOn = 30;
    static const unsigned long usecLedOff = 57;
    static const unsigned long usecAdcOff = 75;
    static const unsigned long usecSpiOff = 80;
    static const unsigned long usecTtl = 100;
};

class TestNephel : public Nephel
{
  public:
    TestNephel(const Pump &goodPump, const Pump &badPump, 
               unsigned long turbidity = 100000, unsigned long goodness = _maxGoodness, 
               unsigned long doubleSeconds = 45 * 60, unsigned long fillSeconds = 10 * 60);
    
    long measure(void);
    void delayScan(void);

  protected:
    unsigned long doubleSeconds(void) { return _doubleSeconds; }
    unsigned long fillSeconds(void) { return _fillSeconds; }

    void update(void);
    unsigned long growthGoodness1k(unsigned long goodness);

    long nephelNoise(void);
  private:
    unsigned long _doubleSeconds;
    unsigned long _fillSeconds;

    const Pump &_goodPump;
    const Pump &_badPump;

    unsigned long _turbidity;
    unsigned long _goodness;
    unsigned long _lastUpdateMsec;
    unsigned long _lastUpdateGoodMsec;
    unsigned long _lastUpdateBadMsec;

    static const unsigned long _maxTurbidity = 2000000;
    static const long _measureFactor = 1000;
    static const long _maxMeasure = 40000;

    static const unsigned long _maxGoodness  = 10000;
    static const unsigned long _goodnessKM   =  2000;
    static const unsigned long _goodnessVmax1k = (1000 * (_maxGoodness + _goodnessKM)) / _maxGoodness;
};

#endif /* defined(_nephelometer_h) */
