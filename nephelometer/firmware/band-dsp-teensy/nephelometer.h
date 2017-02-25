#ifndef _nephelometer_h
#define _nephelometer_h 1

/*
 * Control the nephelometer (turbidity measurement) circuits
 */

#include <ADC.h>
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
    Nephel(int irLedPin = 2, int pgaCSPin = 15, int pgaSCKPin = 13, int pgaMOSIPin = 11, int adcSignalPin = A10, int adcRefPin = A11);
    virtual long measure();
    virtual void delayScan();

    NephelTiming timing(void) { return _timing; }
    void setTiming(NephelTiming timing) { _timing = timing; }

    static const int nPgaScales = 8;
    static const long pgaScales[];

    static long pgaScale(uint8_t setting) { return (setting < nPgaScales) ? pgaScales[setting] : -1; }
    long pgaScale(void) { return _timing.pgaScale(); }
  private:
    int _irLedPin;
    int _pgaCSPin;
    int _pgaSCKPin;
    int _pgaMOSIPin;

    int _adcSignalPin;
    int _adcRefPin;  
    
    SPISettings _pgaSPISettings;
    ADC _adc;

    NephelTiming _timing;
    int setPga(uint8_t setting);
    int measurePeaks(const NephelTiming &, long *ttlon, long *ttloff);
};

class TestNephel : public Nephel
{
  public:
    TestNephel(const Pump &pump, unsigned long turbidity = 100000, unsigned long doubleSeconds = 90 * 60, unsigned long fillSeconds = 20 * 60);
    
    long measure(void);
    void delayScan(void);

  protected:
    unsigned long doubleSeconds(void) { return _doubleSeconds; }
    unsigned long fillSeconds(void) { return _fillSeconds; }

    void update(void);
  private:
    unsigned long _doubleSeconds;
    unsigned long _fillSeconds;

    const Pump &_fillPump;

    unsigned long _turbidity;
    unsigned long _lastUpdateMsec;
    unsigned long _lastUpdatePumpMsec;

    const unsigned long _maxTurbidity = 2000000;
    const long _measureFactor = 1000;
    const long _maxMeasure = 40000;
};

#endif /* defined(_nephelometer_h) */
