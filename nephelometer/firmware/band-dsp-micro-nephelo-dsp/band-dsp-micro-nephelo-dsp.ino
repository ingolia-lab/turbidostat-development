#include <SPI.h>

const int irLedPin = 18; // A0 = PF7 = D18
const int motorPin = 13; // D13 = PC7
const int adcChipSelPin = 23; // A5 = D23 = PF0

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
   Serial.print(F("band-psd-micro nephelometer DSP test 15-07-03\r\n"));
}

const int burnin = 0;
#define NSAMP 64

const int deltaInterval = 0;
int interval = 500;

int motorState = LOW;

void loop() {
  unsigned long start = millis();
  
  motorState = (motorState == LOW) ? HIGH : LOW;
  digitalWrite(motorPin, motorState);

  int adcHigh[NSAMP], adcLow[NSAMP];
  
  for (int i = 0; i < burnin; i++) {
    digitalWrite(irLedPin, HIGH);
    delayMicroseconds(500);
    readAdc();
    digitalWrite(irLedPin, LOW);
    delayMicroseconds(500);
    readAdc();
  }
  
  for (int i = 0; i < NSAMP; i++) {
    digitalWrite(irLedPin, HIGH);
    delayMicroseconds(500);
    adcHigh[i] = readAdc();
    digitalWrite(irLedPin, LOW);
    delayMicroseconds(500);
    adcLow[i] = readAdc();
  }
  
  Serial.print(start);
  Serial.write('\t');
  
  for (int i = 0; i < NSAMP; i++) {  
    Serial.print(adcHigh[i]);
    Serial.write('\t');
  }

  Serial.write('\t');
  
  for (int i = 0; i < NSAMP; i++) {  
    Serial.print(adcLow[i]);
    Serial.write('\t');
  }

  Serial.write("\r\n");

  unsigned long done = millis();
  
  if (done < start + interval) {
    delay((start + interval) - done);    
  }
  interval += deltaInterval;
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

