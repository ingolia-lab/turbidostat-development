#ifndef _turbidostat_h
#define _turbidostat_h 1

class Turbido
{
  public:
    Turbido(Nephel &neph, const NephelMeasure &measure, Pump &pump);
    
    int loop(void);

    static const int eepromStart;
    static const int eepromEnd;

    void readEeprom(void);
    void writeEeprom(void);
    void formatParams(char *buf, unsigned int buflen);
    void manualSetParams(void);

    static const int loopMsec = 1000;
  private:
    Nephel &_neph;
    const NephelMeasure &_measure;
    Pump &_pump;
  
    long _pumpOn;  // Measurement for pump-on
    long _pumpOff; // Measurement for pump-off

    long _prevMsec;
};

#endif /* !defined(_turbidostat_h) */
