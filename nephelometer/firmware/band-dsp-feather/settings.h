#ifndef _settings_h
#define _settings_h 1

class ParamSettings
{
  public:
    virtual void manualSetParams(void);
    virtual void manualReadParams(void) = 0;
    virtual void formatParams(char *buf, unsigned int buflen) = 0;
    virtual void serialWriteParams(void);

  protected:
    static void manualReadLong(const char *desc, long &pval);
    static void manualReadULong(const char *desc, unsigned long &pval);

    static void manualReadPercent(const char *desc, uint8_t &pval);

    static void manualReadPump(const char *desc, uint8_t &pval);

    static void manualReadMeasure(const char *desc, long &mval);
};

#endif /* !defined(_settings_h) */
