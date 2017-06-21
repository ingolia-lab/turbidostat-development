#ifndef _turbidomix_h
#define _turbidomix_h 1

#include "controller.h"
#include "pump.h"
#include "turbidobase.h"

class TurbidoMix : public TurbidoBase
{
  public:
    TurbidoMix(Supervisor &s);

    int begin(void);

    void formatHeader(char *buf, unsigned int buflen);
    void formatLine(char *buf, unsigned int buflen, long currMeasure);

    void formatParams(char *buf, unsigned int buflen);
    void manualReadParams(void);

    const char *name(void) { return "Turbidostat Mix"; }
    char letter(void) { return 'u'; }
  protected:
    Pump &pump1(void);
    Pump &pump2(void);

    void setPumpOn(void);
    void setPump1On(void);
    void setPump2On(void);
    void setPumpOff(void);

    uint8_t pumpCountIncr(void) { unsigned long cycle = _cycleCount++; return (uint8_t) (cycle % 100); }

    virtual uint8_t pump1Percent() { return _pump1Pct; }
  private:
    uint8_t _pump1;
    uint8_t _pump2;
    uint8_t _pump1Pct;

    unsigned long _cycleCount;
};

#endif /* !defined(_turbidomix_h) */
