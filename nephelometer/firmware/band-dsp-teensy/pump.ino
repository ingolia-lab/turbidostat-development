#include "pump.h"

Pump::Pump(int pin, int onIsHigh):
  _pin(pin), _onIsHigh(onIsHigh)
{
  pinMode(_pin, OUTPUT);
  _isPumping = 0;    
  _setPin();  

  _lastOnMsec = -1;
  _lastOffMsec = millis();
  _cumulativeMsec = 0;
}

long Pump::totalOnMsec(void)
{
  return _cumulativeMsec + (_isPumping ? (millis() - _lastOnMsec) : 0);
}

void Pump::setPumping(int newpump)
{
  int oldpump = _isPumping;
  _isPumping = newpump;
  _setPin();

  if (newpump && (!oldpump)) {
    _lastOnMsec = millis();
  } else if ((!newpump) && oldpump) {
    _lastOffMsec = millis();
    _cumulativeMsec += _lastOffMsec - _lastOnMsec;
  }
}

void Pump::reset(void)
{
  setPumping(0);
  _lastOnMsec = -1;
  _lastOffMsec = millis();
  _cumulativeMsec = 0;
}

