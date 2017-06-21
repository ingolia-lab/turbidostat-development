#ifndef _pump_h
#define _pump_h 1

class Pump
{
  public:
    Pump(int pin, int onIsHigh);

    int isPumping(void) const { return _isPumping; }
    long totalOnMsec(void) const;

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

class SyringePump
{
  public:
    // Forward steps, A is high on 1, 2 and B is high on 2, 3
    SyringePump(int pinA, int pinB, long stepMsec = 2);

    long currentStep(void) { return _currStep; }

    void step1Forward(void) { _currStep++; setPins(); }
    void step1Backward(void) { _currStep--; setPins(); }
    void stepN(int nstep);

    long step1Msec(void) { return _stepMsec; }
    long stepNMsec(unsigned int nstep) { return abs(nstep) * _stepMsec; }
  private:
    int _pinA;
    int _pinB;
    long _stepMsec;

    long _currStep;

    int aValue() { return ( (_currStep & 0x03) == 1 || (_currStep & 0x03) == 2) ? HIGH : LOW; }
    int bValue() { return ( (_currStep & 0x03) == 2 || (_currStep & 0x03) == 3) ? HIGH : LOW; }

    void setPins(void) { digitalWrite(_pinA, aValue()); digitalWrite(_pinB, bValue()); delay(_stepMsec); }
};

#endif /* !defined(_pump_h) */
