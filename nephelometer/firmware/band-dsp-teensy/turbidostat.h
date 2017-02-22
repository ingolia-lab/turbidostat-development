#ifndef _turbidostat_h
#define _turbidostat_h 1

class TurbidoParams {
public:
  long pumpOn;  // Measurement for pump-on
  long pumpOff; // Measurement for pump-off

  static const int eepromStart = 32;
  static const int eepromEnd = eepromStart + sizeof(pumpOn) + sizeof(pumpOff);

  void readEeprom(void);
  void writeEeprom(void);
  void format(char *buf, unsigned int buflen);
};

class Turbido
{
  public:
    Turbido(void);
    int loop(void);
  private:
    long startMsec;
    uint8_t pumpState;
    long pumpLastMsec;
    long pumpTotalMsec;
    TurbidoParams params;
};

#endif /* !defined(_turbidostat_h) */
