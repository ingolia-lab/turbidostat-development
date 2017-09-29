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

struct half_calib_struct {
  int v;
  int ref;
  unsigned long startUsec;
  unsigned long ledUsec;
  unsigned long refSampleUsec;
  unsigned long refAdcUsec;
  unsigned long vSampleUsec;
  unsigned long vAdcUsec;
};

struct calib_struct {
  struct half_calib_struct on;
  struct half_calib_struct off;
};

#define NCALIB 24
struct calib_struct calibs[NCALIB];

ADC *adc = new ADC();

void setup() {
  // put your setup code here, to run once:

  pinMode(irLedPin, OUTPUT);
  digitalWrite(irLedPin, HIGH);

  pinMode(pgaCSPin, OUTPUT);
  digitalWrite(pgaCSPin, HIGH);

  pinMode(pgaSCKPin, OUTPUT);
  pinMode(pgaMOSIPin, OUTPUT);

  Serial.begin(9600);

  SPI.setSCK(pgaSCKPin);
  SPI.begin();

  pinMode(A10, INPUT);
  pinMode(A11, INPUT);

  adc->setAveraging(1);
  adc->setResolution(12);
  adc->setConversionSpeed(ADC_HIGH_SPEED);
  adc->setSamplingSpeed(ADC_MED_SPEED);
}

const unsigned long manualMeasureTime = 2000;

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long tstart = millis();

  SPI.beginTransaction(pgaSettings);
  digitalWrite(pgaCSPin, LOW);
  SPI.transfer(0x40); 
  SPI.transfer(0);
  digitalWrite(pgaCSPin, HIGH);
  SPI.endTransaction();

  calibrate(calibs, NCALIB);
  printCalibs();
  
  unsigned long tdone = millis();
  if (tdone < tstart + manualMeasureTime) {
    delay((tstart + manualMeasureTime) - tdone); 
  }
}

void printCalibs()
{
  for (int i = 0; i < NCALIB; i++) {
    Serial.print(i);
    Serial.write('\t');
    printCalib(&(calibs[i]));
  }

  for (int i = 1; i < NCALIB; i++) {
    Serial.print(calibs[i].on.startUsec - calibs[i-1].on.startUsec);
    Serial.write('\t');
  }
  Serial.println();
}

void printCalib(struct calib_struct *c)
{
  Serial.print(c->on.startUsec);
  Serial.write('\t');
  Serial.print(c->on.v);
  Serial.write('\t');
  Serial.print(c->off.v);
  Serial.write('\t');
  Serial.print(c->on.ref);
  Serial.write('\t');
  Serial.print(c->off.ref);

  Serial.write('\t');
  Serial.print(c->on.ledUsec - c->on.startUsec);
  Serial.write('\t');
  Serial.print(c->on.refSampleUsec - c->on.startUsec);
  Serial.write('\t');
  Serial.print(c->on.refAdcUsec - c->on.startUsec);
  Serial.write('\t');
  Serial.print(c->on.vSampleUsec - c->on.startUsec);
  Serial.write('\t');
  Serial.print(c->on.vAdcUsec - c->on.startUsec);
  Serial.write('\t');
  Serial.print(c->off.startUsec - c->on.startUsec);
  Serial.write('\t');
  Serial.print(c->off.vSampleUsec - c->on.startUsec);
  
  Serial.println();
}

const int halfPeriodUsec = 50;
const int referenceUsec = 10;
const int measureUsec = 25;

void calibrate(struct calib_struct calibs[], int ncalib)
{
  for (int i = 0; i < ncalib; i++) {
    calibrate_half(&(calibs[i].on), LOW);
    calibrate_half(&(calibs[i].off), HIGH);
  }
}

inline void delayIfNeeded(unsigned long until)
{
  unsigned long now = micros();
  if (now < until) {
    delayMicroseconds(until - now);
  }
}

void calibrate_half(struct half_calib_struct *c, int led)
{
    unsigned long tStart = micros();    
    c->startUsec = tStart;

    digitalWrite(irLedPin, led);
    c->ledUsec = micros();
    
    delayIfNeeded(tStart + referenceUsec);
    c->refSampleUsec = micros();
    
    c->ref = adc->analogRead(A11);
    c->refAdcUsec = micros();

    delayIfNeeded(tStart + referenceUsec);
    c->vSampleUsec = micros();
    
    c->v = adc->analogRead(A10);
    c->vAdcUsec = micros();

    delayIfNeeded(tStart + halfPeriodUsec);
}

const int burnin = 16;
const int nsamp = 18;
const int delayUsec = 50; 
// 128 cycles at 1040 microseconds / cycle = 0.133 seconds, covers 7.98 cycles @ 60 Hz
// 256 cycles at 1040 microseconds / cycle = 0.266 seconds, covers 16.0 cycles @ 60 Hz


void pulseWithPGA(void)
{
  for (int i = 0; i < burnin; i++) {
    digitalWrite(irLedPin, LOW);
    delayMicroseconds(delayUsec);
    digitalWrite(irLedPin, HIGH);
    delayMicroseconds(delayUsec);
  }
  
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
    digitalWrite(irLedPin, HIGH);
    delayMicroseconds(delayUsec);
  }
}
