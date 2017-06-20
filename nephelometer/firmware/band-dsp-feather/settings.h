#ifndef _settings_h
#define _settings_h 1

class ParamSettings
{
  public:
    virtual void manualSetParams(void);
    virtual void formatParams(char *buf, unsigned int buflen);
    void serialWriteParams(void);

  protected:
    static void manualReadParam(const char *desc, long &pval);
    static void manualReadParam(const char *desc, unsigned long &pval);
    static void manualReadParam(const char *desc, unsigned int &pval);

    static void manualReadPercent(const char *desc, uint8_t &pval);

    static void manualReadPump(const char *desc, uint8_t &pval);

};

#endif /* !defined(_settings_h) */
