#include <ADC.h>
#include <EEPROM.h>
#include <SPI.h>

const int irLedPin = 2;

const int pgaCSPin = 15;
const int pgaSCKPin = 13;
const int pgaMOSIPin = 11;

// MODE3 = 1,1 is better so device select pin doesn't clock
SPISettings pgaSettings(4000000 /* 4 MHz */, MSBFIRST, SPI_MODE3);

// const int motorPin = XXX
// const int adcChipSelPin = XXX

struct measure_struct {
  long von;
  long voff;
  long vref;
  int nsamp;
};

struct measure_fast_struct {
  long v;
  unsigned long when;
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

#define NCALIB 16
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
  adc->setSamplingSpeed(ADC_HIGH_SPEED);

  SPI.beginTransaction(pgaSettings);
  digitalWrite(pgaCSPin, LOW);
  SPI.transfer(0x40); 
  SPI.transfer(0);
  digitalWrite(pgaCSPin, HIGH);
  SPI.endTransaction();
}

const unsigned long manualMeasureTime = 100000;
/*
#define NMEASURE 167
#define LONGMEASURE 2000

struct measure_fast_struct measures[LONGMEASURE];
*/
int running_nmeasure = 2;

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long tStart = micros();

  Serial.print(tStart / ((unsigned long) 1000000));
  Serial.write('.');
  Serial.print(tStart / ((unsigned long)  100000) % 10);
  Serial.print(tStart / ((unsigned long)   10000) % 10);

  measureRunningAndPrint(running_nmeasure);
  running_nmeasure += running_nmeasure;
  if (running_nmeasure > 512) {
    running_nmeasure = 2;
  }

  delayIfNeeded(tStart + manualMeasureTime);

/*
  measureFast(measures, LONGMEASURE, 0x03);
  printMeasureFastTable(measures, LONGMEASURE);
*/
/*
  struct measure_struct measuresLow[NMEASURE], measuresMed[NMEASURE], measuresHigh[NMEASURE];
  measure(measuresLow, NMEASURE, 0x00);  
  measure(measuresMed, NMEASURE, 0x03);
  measure(measuresHigh, NMEASURE, 0x07);
  printMeasureValue(measuresLow, NMEASURE);
  printMeasureValue(measuresMed, NMEASURE);
  printMeasureValue(measuresHigh, NMEASURE);
  Serial.println();
*/

/*
  printMeasure(measures, NMEASURE);
*/
/*
  calibrate(calibs, NCALIB);
  printCalibs();
*/
/*
  runPulses(10, 50);
*/
/*
  onlineMeasure();
*/
}

#define MEASURE_EQUIL 8
#define MEASURE_HALFT ((unsigned long) 50)
#define MEASURE_ADCT  ((unsigned long) 22)

void measure(struct measure_struct measures[], unsigned int nmeasures, uint8_t pgaSetting)
{
  SPI.beginTransaction(pgaSettings);
  digitalWrite(pgaCSPin, LOW);   
  SPI.transfer(0x40); 
  SPI.transfer(pgaSetting);
  digitalWrite(pgaCSPin, HIGH);
  SPI.endTransaction();
  
  unsigned long tStart = micros();
  
  for (unsigned int i = 0; i < MEASURE_EQUIL; i++) {
    digitalWrite(irLedPin, LOW);
    delayIfNeeded(tStart + (1 + 2 * i) * MEASURE_HALFT);
    digitalWrite(irLedPin, HIGH);
    delayIfNeeded(tStart + (2 + 2 * i) * MEASURE_HALFT);
  }

  unsigned long tMeasure = tStart + 2 * MEASURE_EQUIL * MEASURE_HALFT;

  for (unsigned int j = 0; j < nmeasures; j++) {
    unsigned long tCycle = tMeasure + 2 * j * MEASURE_HALFT;
    digitalWrite(irLedPin, LOW);
    delayIfNeeded(tCycle + MEASURE_ADCT);
    measures[j].von = adc->analogRead(A10);
    measures[j].vref = adc->analogRead(A11);
    delayIfNeeded(tCycle + MEASURE_HALFT);
    digitalWrite(irLedPin, HIGH);
    delayIfNeeded(tCycle + MEASURE_HALFT + MEASURE_ADCT);
    measures[j].voff = adc->analogRead(A10);
    delayIfNeeded(tCycle + 2 * MEASURE_HALFT);
  }
}

void measureFast(struct measure_fast_struct measures[], unsigned int nmeasures, uint8_t pgaSetting)
{
  SPI.beginTransaction(pgaSettings);
  digitalWrite(pgaCSPin, LOW);   
  SPI.transfer(0x40); 
  SPI.transfer(pgaSetting);
  digitalWrite(pgaCSPin, HIGH);
  SPI.endTransaction();
  
  unsigned long tStart = micros();
  
  for (unsigned int i = 0; i < MEASURE_EQUIL; i++) {
    digitalWrite(irLedPin, LOW);
    delayIfNeeded(tStart + (1 + 2 * i) * MEASURE_HALFT);
    digitalWrite(irLedPin, HIGH);
    delayIfNeeded(tStart + (2 + 2 * i) * MEASURE_HALFT);
  }

  for (unsigned int j = 0; j < nmeasures; j++) {
    unsigned long when = micros();
    if ((when - tStart) % (2 * MEASURE_HALFT) < MEASURE_HALFT) {
      digitalWrite(irLedPin, LOW);
    } else {
      digitalWrite(irLedPin, HIGH);
    }
    
    measures[j].v = adc->analogRead(A10);
    measures[j].when = when;
  }

  digitalWrite(irLedPin, HIGH);
}

void printMeasures(struct measure_struct measures[], unsigned int nmeasures)
{
  Serial.print(micros());
  for (unsigned int j = 0; j < nmeasures; j++) {
    Serial.write('\t');
    Serial.print(measures[j].vref - measures[j].von);
    Serial.write('\t');
    Serial.print(measures[j].voff - measures[j].vref);
  }
  Serial.println();
}

void printMeasureTable(struct measure_struct measures[], unsigned int nmeasures)
{
  Serial.println(micros());
  for (unsigned int j = 0; j < nmeasures; j++) {
    Serial.write('\t');
    Serial.print(measures[j].von);
    Serial.write('\t');
    Serial.print(measures[j].voff);
    Serial.write('\t');
    Serial.println(measures[j].vref);
  }
}

void printMeasureFastTable(struct measure_fast_struct measures[], unsigned int nmeasures)
{
  for (unsigned int j = 0; j < nmeasures; j++) {
    Serial.print(measures[j].when);
    Serial.write('\t');
    Serial.println(measures[j].v);
  }
}

void printMeasure(struct measure_struct measures[], unsigned int nmeasures)
{
  long on = 0, off = 0;
  for (unsigned int j = 0; j < nmeasures; j++) {
    on  += measures[j].vref - measures[j].von;
    off += measures[j].voff - measures[j].vref;
  }
  Serial.print(micros() / ((unsigned long) 1000000));
  Serial.write('\t');
  Serial.print((on + off) / (2 * (long) nmeasures));
  Serial.write('\t');
  Serial.print(on);
  Serial.write('\t');
  Serial.print(off);
  Serial.println(); 
}

void printMeasureValue(struct measure_struct measures[], unsigned int nmeasures)
{
  long on = 0, off = 0;
  for (unsigned int j = 0; j < nmeasures; j++) {
    on  += measures[j].vref - measures[j].von;
    off += measures[j].voff - measures[j].vref;
  }
  Serial.write('\t');
  Serial.print((on + off) / (2 * (long) nmeasures));
}


#define RUNNING_FIXED 10

struct running_struct {
  long n;
  /* All fixed point multiplied by RUNNING_FIXED */
  long oldM, newM, oldS, newS;
};

void add_to_running(struct running_struct *r, long xin)
{
  long x = xin * RUNNING_FIXED;
  r->n++;
  if (r->n == 1) {
    r->oldM = r->newM = x;
    r->oldS = 0;
  } else {
    r->newM = r->oldM + (x - r->oldM) / r->n;
    r->newS = r->oldS + (x - r->oldM) * (x - r->newM);
    r->oldM = r->newM;
    r->oldS = r->newS;
  }
}

long running_mean_unscaled(const struct running_struct *r)
{
  return (r->n > 0) ? r->newM : 0;
}

long running_var_unscaled(const struct running_struct *r)
{
  return ( (r->n > 1) ? r->newS/(r->n - 1) : 0 ); 
}

void runningMeasure(struct running_struct *r, unsigned int nmeasures, long *ttlon, long *ttloff, long *ttlx, long *ttlx2, long *ttlref)
{
  r->n = 0;
  *ttlon = 0;
  *ttloff = 0;
  *ttlx = 0;
  *ttlx2 = 0;
  *ttlref = 0;

  SPI.beginTransaction(pgaSettings);
  digitalWrite(pgaCSPin, LOW);   
  SPI.transfer(0x40); 
  SPI.transfer(0x02);
  digitalWrite(pgaCSPin, HIGH);
  SPI.endTransaction();
  
  unsigned long tStart = micros();
  
  for (unsigned int i = 0; i < MEASURE_EQUIL; i++) {
    digitalWrite(irLedPin, LOW);
    delayIfNeeded(tStart + (1 + 2 * i) * MEASURE_HALFT);
    digitalWrite(irLedPin, HIGH);
    delayIfNeeded(tStart + (2 + 2 * i) * MEASURE_HALFT);
  }

  unsigned long tMeasure = tStart + 2 * MEASURE_EQUIL * MEASURE_HALFT;

  for (unsigned int j = 0; j < nmeasures; j++) {
    unsigned long tCycle = tMeasure + 2 * j * MEASURE_HALFT;
    digitalWrite(irLedPin, LOW);    

    delayIfNeeded(tCycle + MEASURE_ADCT);
    long von = adc->analogRead(A10);
    long vref = adc->analogRead(A11);
    *ttlon += von;
    *ttlref += vref;

    delayIfNeeded(tCycle + MEASURE_HALFT);
    digitalWrite(irLedPin, HIGH);

    delayIfNeeded(tCycle + MEASURE_HALFT + MEASURE_ADCT);
    long voff = adc->analogRead(A10);
    *ttloff += voff;

    long x = (voff - von);
    add_to_running(r, x);    
    *ttlx += x;
    *ttlx2 += x * x;
    
    delayIfNeeded(tCycle + 2 * MEASURE_HALFT);
  }
}

void measureRunningAndPrint(unsigned int nmeasure)
{
  struct running_struct r;
  long ttlon, ttloff, ttlx, ttlx2, ttlref;

  runningMeasure(&r, nmeasure, &ttlon, &ttloff, &ttlx, &ttlx2, &ttlref);

  long var = (ttlx2 / ((long) nmeasure)) - (ttlx / ((long) nmeasure))*(ttlx / ((long) nmeasure));

  Serial.write('\t');
  Serial.print(r.n);
  Serial.write('\t');
  Serial.print((ttloff - ttlon) / ((long) nmeasure));
  Serial.write('\t');
  Serial.print(ttlx / ((long) nmeasure));
  Serial.write('\t');
  Serial.print(running_mean_unscaled(&r));
  Serial.write('\t');
  Serial.print(ttlref / ((long) nmeasure));
  Serial.write('\t');
  Serial.print((ttloff + ttlon - 2 * ttlref) / (2 * (long) nmeasure));
  Serial.write('\t');
  Serial.print(ttlx2 / ((long) nmeasure));
  Serial.write('\t');
  Serial.print(var);
  Serial.write('\t');
  Serial.print(running_var_unscaled(&r));
  Serial.write('\t');
  Serial.println(sqrt(var));
}

#define ONLINE_MEASURE_TIME 62500
#define ONLINE_MEASURE_N 8
#define ONLINE_NMEASURE 512

void onlineMeasure()
{
  long onlineXTtl = 0;
  unsigned long tStart = micros();

  for (int i = 0; i < ONLINE_MEASURE_N; i++) {
    delayIfNeeded(tStart + i * ONLINE_MEASURE_TIME);
    struct running_struct r;
    long ttlon, ttloff, ttlx, ttlx2, ttlref;

    runningMeasure(&r, ONLINE_NMEASURE, &ttlon, &ttloff, &ttlx, &ttlx2, &ttlref);

    onlineXTtl += ttlx;
  }

  Serial.print(tStart / ((unsigned long) 1000000));
  Serial.write('.');
  Serial.print(tStart / ((unsigned long)  100000) % 10);
  Serial.print(tStart / ((unsigned long)   10000) % 10);

  Serial.write('\t');
  Serial.println(onlineXTtl / ((long) 5 * ONLINE_NMEASURE * ONLINE_MEASURE_N));

  delayIfNeeded(tStart + ONLINE_MEASURE_TIME * ONLINE_MEASURE_N);
}

void runPulseSequence()
{
    runPulses(20000 * 1/3, 50 * 3);
    runPulses(20000 * 1/2, 50 * 2);
    runPulses(20000 * 2/3, 50 * 3/2);
    runPulses(20000 * 3/4, 50 * 4/3);
    runPulses(20000, 50);
    runPulses(20000 * 4/3, 50 * 3/4);
    runPulses(20000 * 3/2, 50 * 2/3);
    runPulses(20000 * 2,   50 * 1/2);
    runPulses(20000 * 3,   50 * 1/3);
}

void runPulses(int nPulses, unsigned long halfTUsec)
{
  for (int i = 0; i < nPulses; i++) {  
    unsigned long tStart = micros();   

    digitalWrite(irLedPin, HIGH);
    delayIfNeeded(tStart + halfTUsec);
    digitalWrite(irLedPin, LOW);
    delayIfNeeded(tStart + 2 * halfTUsec);
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
