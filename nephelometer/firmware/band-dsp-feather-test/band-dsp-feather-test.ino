#include <SPI.h>

static const int ledPin = 17;
static const int mot1Pin = 18;
static const int mot2Pin = 19;
static const int sckPin = 24;
static const int mosiPin = 23;
static const int misoPin = 22;
static const int mot3Pin = 10;
static const int mot4Pin = 11;
static const int csAdc = 6;
static const int csPga = 5;

SPISettings pgaSPISettings(4000000 /* 4 MHz */, MSBFIRST, SPI_MODE3);
SPISettings adcSPISettings(2000000 /* 2 MHz */, MSBFIRST, SPI_MODE0);

#define BUFLEN 256
static char buffer[BUFLEN];

inline unsigned long delayIfNeeded(unsigned long until)
{
  unsigned long now = micros();
  if (now < until) {
    delayMicroseconds(until - now);
    return until - now;
  } else {
    return 0;
  }
}

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  pinMode(mot1Pin, OUTPUT);
  digitalWrite(mot1Pin, HIGH);
  pinMode(mot2Pin, OUTPUT);
  digitalWrite(mot2Pin, HIGH);
  pinMode(sckPin, OUTPUT);
  digitalWrite(sckPin, HIGH);
  pinMode(mosiPin, OUTPUT);
  digitalWrite(mosiPin, LOW);
  pinMode(misoPin, INPUT);
  pinMode(mot3Pin, OUTPUT);
  digitalWrite(mot3Pin, HIGH);
  pinMode(mot4Pin, OUTPUT);
  digitalWrite(mot4Pin, HIGH);
  pinMode(csAdc, OUTPUT);
  digitalWrite(csAdc, HIGH);
  pinMode(csPga, OUTPUT);
  digitalWrite(csPga, HIGH);
  
  Serial.begin(9600);
  SPI.begin();

  SPI.beginTransaction(pgaSPISettings);
  digitalWrite(csPga, LOW);
  SPI.transfer(0x40);
  SPI.transfer(0x02);
  digitalWrite(csPga, HIGH);
  SPI.endTransaction();
}

#define NPRE  8
#define NSAMP 32  

void loop() {
  long samplesOn[NSAMP], samplesOff[NSAMP];
  
  unsigned long startUsec = micros();
  for (int i = 0; i < NPRE + NSAMP; i++) {
    digitalWrite(ledPin, LOW);
    // 7 µs delay before LED turns on
    // 30 µs delay to peak
    delayIfNeeded(startUsec + i*100 + 25);
    SPI.beginTransaction(adcSPISettings);
    digitalWrite(csAdc, LOW);
    delayIfNeeded(startUsec + i*100 + 30);
    long sample = SPI.transfer16(0x0000);
    if (i >= NPRE) {
      samplesOn[i - NPRE] = sample & 0xfff;
    }
    digitalWrite(csAdc, HIGH);
    SPI.endTransaction();

    delayIfNeeded(startUsec + i*100 + 57);    
    digitalWrite(ledPin, HIGH);

    delayIfNeeded(startUsec + i*100 + 75);
    SPI.beginTransaction(adcSPISettings);
    digitalWrite(csAdc, LOW);
    delayIfNeeded(startUsec + i*100 + 80);
    sample = SPI.transfer16(0x0000);
    if (i >= NPRE) {
      samplesOff[i - NPRE] = sample & 0xfff;
    }
    digitalWrite(csAdc, HIGH);
    SPI.endTransaction();    
    
    delayIfNeeded(startUsec + (i+1)*100);
  }
  
  if (1) {
    for (int step = 0; step < 50; step++) {
      for (int cycle = 0; cycle < 4; cycle++) {
        digitalWrite(mot3Pin, (cycle < 2) ? HIGH : LOW);
        digitalWrite(mot4Pin, (cycle > 0 & cycle < 3) ? HIGH : LOW);
        delay(2);
      }
    }
  }

  long ttlOn = 0, ttlOff = 0;
  for (int i = 0; i < NSAMP; i++) {
    ttlOn += samplesOn[i];
    ttlOff += samplesOff[i];
  }

  long avg1000 = (10 * (ttlOff - ttlOn)) / NSAMP;
  
  snprintf(buffer, BUFLEN, "%ld.%03ld\t%ld\t%ld\r\n", (avg1000 / 1000), (avg1000 % 1000), ttlOn, ttlOff);
  Serial.write(buffer);
  
  delayIfNeeded(startUsec + 1000000);
}
