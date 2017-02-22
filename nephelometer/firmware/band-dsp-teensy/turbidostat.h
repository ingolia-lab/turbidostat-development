#ifndef _turbidostat_h
#define _turbidostat_h 1

class Turbido
{
  public:
    Turbido(void);
    int loop(void);

    static const int eepromStart;
    static const int eepromEnd;

    void readEeprom(void);
    void writeEeprom(void);
    void formatParams(char *buf, unsigned int buflen);

  private:
    long _pumpOn;  // Measurement for pump-on
    long _pumpOff; // Measurement for pump-off

    long _startMsec;
    int _pumpState;
    long _pumpLastMsec;
    long _pumpTotalMsec;
};

#endif /* !defined(_turbidostat_h) */
