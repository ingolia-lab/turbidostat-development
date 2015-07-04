#include <SPI.h>

const int irLedPin = 18; // A0 = PF7 = D18
const int motorPin = 13; // D13 = PC7
const int adcChipSelPin = 23; // A5 = D23 = PF0

int motorState = LOW;

struct measure_struct {
  long von;
  long voff;
  int nsamp;
};

void setup() {
  pinMode(adcChipSelPin, OUTPUT);
  digitalWrite(adcChipSelPin, HIGH);

  pinMode(irLedPin, OUTPUT);
  digitalWrite(irLedPin, LOW);
  
  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);
  
  SPI.setClockDivider( SPI_CLOCK_DIV16 );
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.begin();
   
  Serial.begin(9600);
  Serial.print(F("band-psd-micro 15-07-03\r\n"));
}

const int burnin = 32;
const int nsamp = 96;
const int delayUsec = 500;

void measure(struct measure_struct *result)
{
  if (motorState == LOW) { motorState = HIGH; } else { motorState = LOW; }
  digitalWrite(motorPin, motorState); 
  
  for (int i = 0; i < burnin; i++) {
    digitalWrite(irLedPin, HIGH);
    delayMicroseconds(delayUsec);
    readAdc();
    digitalWrite(irLedPin, LOW);
    delayMicroseconds(delayUsec);
    readAdc();
  }
  
  result->von = 0;
  result->voff = 0;
  result->nsamp = 0;
  for (int i = 0; i < nsamp; i++) {
    digitalWrite(irLedPin, HIGH);
    delayMicroseconds(delayUsec);
    result->von += ((long) readAdc());
    digitalWrite(irLedPin, LOW);
    delayMicroseconds(delayUsec);
    result->voff += ((long) readAdc());
    result->nsamp++;
  }
}

void loop() {
  unsigned long start = millis();
  
  struct measure_struct m;

  measure(&m);
  
  long value = ((m.voff) - (m.von)) / ((long) m.nsamp);

  Serial.print(start);
  Serial.write('\t');
  Serial.print(m.von);
  Serial.write('\t');
  Serial.print(m.voff);
  Serial.write('\t');
  Serial.print(m.nsamp);  
  Serial.write('\t');
  Serial.print(value);  
  Serial.write("\r\n");

  unsigned long done = millis();
  
  if (done < start + 1000) {
    delay((start + 1000) - done);    
  }
}

int readAdc(void) {
  int adcval = 0;
  digitalWrite(adcChipSelPin, LOW);
  int b1 = SPI.transfer(0);
  int b2 = SPI.transfer(0);
  digitalWrite(adcChipSelPin, HIGH);

  int sign = b1 & B00010000;
  int hi =   b1 & B00001111;
  int lo =   b2;
  
  int reading = hi * 256 + lo;
  if (sign) { reading = (4096 - reading) * -1; }

/*  
  Serial.print(b1);
  Serial.write('\t');
  Serial.print(b2);
  Serial.write('\t');
  */
  return reading;
}

