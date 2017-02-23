#ifndef _settings_h
#define _settings_h 1

class ParamSettings
{
  public:
    virtual void readEeprom(unsigned int);
    virtual void writeEeprom(unsigned int);
    virtual void manualSetParams(void);
    virtual void formatParams(char *buf, unsigned int buflen);
    void serialWriteParams(void);

  protected:
    static void writeEepromLong(unsigned int base, unsigned int slot, long value);
    static long readEepromLong(unsigned int base, unsigned int slot);
    static void manualReadParam(const char *desc, long &pval);
    static void manualReadParam(const char *desc, unsigned long &pval);
    static void manualReadParam(const char *desc, unsigned int &pval);
};

#endif /* !defined(_settings_h) */
