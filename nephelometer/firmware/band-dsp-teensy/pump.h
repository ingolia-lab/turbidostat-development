#ifndef _pump_h
#define _pump_h 1

class Pump
{
  public:
    Pump(int pin, int onIsHigh);

    int isPumping(void) { return _isPumping; }
    long totalOnMsec(void);

    void setPumping(int newpump);

    void reset(void);
  private:
    int _pin;
    int _onIsHigh; // != 0 means HIGH turns on; == 0 means LOW turns on
    inline int _onValue() { return _onIsHigh ? HIGH : LOW; }
    inline int _offValue() { return _onIsHigh ? LOW : HIGH; }

    // If _isPumping then 
    int _isPumping;
    long _lastOnMsec;
    long _lastOffMsec;
    long _cumulativeMsec;

    inline int _pinValue() { return _isPumping ? _onValue() : _offValue(); }
    inline void _setPin(void) { digitalWrite(_pin, _pinValue()); }
};

#endif /* !defined(_pump_h) */
