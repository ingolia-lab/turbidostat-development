#ifndef _nephelometer_h
#define _nephelometer_h 1

#include <ADC.h>
#include <SPI.h>

class NephelMeasure {
public:
  NephelMeasure();
  
  uint8_t pga;
  unsigned long halfCycleUsec;
  unsigned long adcDelayUsec;
  
  unsigned int nEquil;
  unsigned int nMeasure;

  long pgaScale(void);
};

class Nephel
{
  public:
    Nephel(int irLedPin = 2, int pgaCSPin = 15, int pgaSCKPin = 13, int pgaMOSIPin = 11, int adcSignalPin = A10, int adcRefPin = A11);
    long measure(const NephelMeasure &);
    void delayScan();

    static const int nPgaScales = 8;
    static const long pgaScales[];

    static long pgaScale(uint8_t setting) { return (setting < nPgaScales) ? pgaScales[setting] : -1; }
  private:
    int _irLedPin;
    int _pgaCSPin;
    int _pgaSCKPin;
    int _pgaMOSIPin;

    int _adcSignalPin;
    int _adcRefPin;  
    
    SPISettings _pgaSPISettings;
    ADC _adc;
    int setPga(uint8_t setting);
    int measurePeaks(const NephelMeasure &, long *ttlon, long *ttloff);
};

#endif /* defined(_nephelometer_h) */
