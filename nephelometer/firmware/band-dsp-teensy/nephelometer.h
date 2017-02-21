#ifndef _nephelometer_h
#define _nephelometer_h 1

#include <ADC.h>
#include <SPI.h>

struct nephel_hw_struct {
  int irLedPin;
  int pgaCSPin;
  int pgaSCKPin;
  int pgaMOSIPin;

  int adcSignalPin;
  int adcRefPin;
};

extern struct nephel_hw_struct hwDefault;

struct nephelometer_struct;

struct nephel_measure_struct {
  uint8_t pga;
  unsigned long halfCycleUsec;
  unsigned long adcDelayUsec;
  unsigned int nEquil;
  unsigned int nMeasure;  
};

extern struct nephel_measure_struct measureDefault;

struct nephelometer_struct *nephel_init(const struct nephel_hw_struct *hw);
extern const int nephel_pga_nsettings;
long pgaScale(uint8_t setting);

int nephel_measure(struct nephelometer_struct *neph, const struct nephel_measure_struct *measure, long *avg10);
void neph_delayScan(struct nephelometer_struct *neph);

#endif /* defined(_nephelometer_h) */
