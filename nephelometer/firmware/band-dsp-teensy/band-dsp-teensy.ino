#include <ADC.h>
#include <EEPROM.h>
#include <SPI.h>

const int irLedPin = 2;

const int pgaCSPin = 15;
const int pgaSCKPin = 14;
const int pgaMOSIPin = 11;

// MODE3 = 1,1 is better so device select pin doesn't clock
SPISettings pgaSettings(4000000 /* 4 MHz */, MSBFIRST, SPI_MODE3);

// const int motorPin = XXX
// const int adcChipSelPin = XXX

struct measure_struct {
  long von;
  long voff;
  int nsamp;
};

void setup() {
  // put your setup code here, to run once:

  pinMode(irLedPin, OUTPUT);
  digitalWrite(irLedPin, HIGH);

  pinMode(pgaCSPin, OUTPUT);
  digitalWrite(pgaCSPin, HIGH);

  pinMode(pgaSCKPin, OUTPUT);
  pinMode(pgaMOSIPin, OUTPUT);

  Serial.begin(9600);

  Serial.write("about to begin SPI and loop...");

  SPI.setSCK(pgaSCKPin);
  SPI.begin();
}

const unsigned long manualMeasureTime = 500;

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long tstart = millis();
  struct measure_struct m;
  measure(&m);

    
  unsigned long tdone = millis();
  if (tdone < tstart + manualMeasureTime) {
    delay((tstart + manualMeasureTime) - tdone); 
  }
}

const int burnin = 16;
const int nsamp = 18;
const int delayUsec = 50; 
// 128 cycles at 1040 microseconds / cycle = 0.133 seconds, covers 7.98 cycles @ 60 Hz
// 256 cycles at 1040 microseconds / cycle = 0.266 seconds, covers 16.0 cycles @ 60 Hz

void measure(struct measure_struct *result)
{
  for (int i = 0; i < burnin; i++) {
    digitalWrite(irLedPin, LOW);
    delayMicroseconds(delayUsec);
//    readAdc();
    digitalWrite(irLedPin, HIGH);
    delayMicroseconds(delayUsec);
//    readAdc();
  }
  
  result->von = 0;
  result->voff = 0;
  result->nsamp = 0;
  for (int i = 0; i < nsamp; i++) {
    uint8_t g = i % 9;
    SPI.beginTransaction(pgaSettings);
    digitalWrite(pgaCSPin, LOW);
    if (g < 8) {    
      SPI.transfer(0x40); 
      SPI.transfer(g);
    } else {
      SPI.transfer(0x20);
      SPI.transfer(0);
    }
    digitalWrite(pgaCSPin, HIGH);
    SPI.endTransaction();
    
    digitalWrite(irLedPin, LOW);
    delayMicroseconds(delayUsec);
//    result->von += ((long) readAdc());
    digitalWrite(irLedPin, HIGH);
    delayMicroseconds(delayUsec);
//    result->voff += ((long) readAdc());
    result->nsamp++;
  }
}
