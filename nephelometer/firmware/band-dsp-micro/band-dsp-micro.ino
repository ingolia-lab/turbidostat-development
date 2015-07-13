#include <EEPROM.h>
#include <SPI.h>

const int irLedPin = 18; // A0 = PF7 = D18
const int motorPin = 13; // D13 = PC7
const int adcChipSelPin = 23; // A5 = D23 = PF0

struct param_struct {
  long pumpOnV; // Raw V measurement for pump-on
  long pumpOffV; // Raw V measurement for pump-off
  long volume; // Total volume of pump chamber, in pump-seconds
  long calibration; // Raw V measurement for OD = 1.0
};

struct measure_struct {
  long von;
  long voff;
  int nsamp;
};

enum state_enum {
  STATE_MANUAL,
  STATE_TURBIDO_START,
  STATE_TURBIDO_RUN,
  STATE_TURBIDO_STOP
};

struct turbido_struct {
  enum state_enum state;
  int pumpState;
  long pumpLastTime;
  long pumpTotalTime;
  struct param_struct params;
};

struct turbido_struct turbido;

long tnext;
const long tstep = 1000;

int manualIdleAutoStart = 1;
long manualIdleTimeout = 90 * (long) 1000;

const int outbuf_len = 192;
char outbuf[outbuf_len];

void setup() {
  pinMode(adcChipSelPin, OUTPUT);
  digitalWrite(adcChipSelPin, HIGH);

  pinMode(irLedPin, OUTPUT);
  digitalWrite(irLedPin, LOW);
  
  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);
  
  SPI.setClockDivider( SPI_CLOCK_DIV16 ); // 1 MHz SPI clock
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.begin();
   
  turbido.pumpState = LOW;
  turbido.pumpLastTime = -1;
  turbido.pumpTotalTime = 0;
  turbido.state = STATE_MANUAL;

  Serial.begin(9600);
  Serial.print(F("band-psd-micro 15-07-13\r\n"));
}

void loop() {
  switch(turbido.state) {
    case STATE_MANUAL:
    {
      manualLoop();
    }      
    break;

    case STATE_TURBIDO_START:
    {
      turbido.state = STATE_TURBIDO_RUN;
      tnext = (millis() / tstep) + 2;
      Serial.println(F("\r\n# Turbidostat mode (q to quit)"));
      
      readEepromParams(&(turbido.params));
      formatEepromParams(outbuf, outbuf_len, &(turbido.params));
      Serial.write(outbuf);
    }
    break;
    
    case STATE_TURBIDO_RUN:
    {
      turbidoLoop();
    }
    break;
    
    case STATE_TURBIDO_STOP:
    {
      turbido.state = STATE_MANUAL;
      turbido.pumpState = LOW;
      turbido.pumpLastTime = -1;
      digitalWrite(motorPin, turbido.pumpState);
    }
    break;

    default:
    {
      Serial.print(F("\r\n### UNKNOWN STATE\r\n"));
      delay(1000);
      turbido.state = STATE_MANUAL; 
    }
  };

}

void manualLoop()
{
  Serial.print(F("# setup [amprst] > "));

  int cmd;
  unsigned long idleStart = millis();
  
  while ((cmd = Serial.read()) < 0) {
    delay(1);
    if (manualIdleAutoStart && millis() > manualIdleTimeout + idleStart) {
      Serial.print(F("manual idle timeout, starting turbidostat\r\n"));
      turbido.state = STATE_TURBIDO_START;
      return;
    }
  }
  
  manualIdleAutoStart = 0;
  
  Serial.write(cmd);
  
  switch(cmd) {
    case 'a':
      manualAnnotate();
      break;
      
    case 'm':
      manualMeasure();
      break;

    case 'p':
      manualPump();
      break;

    case 'r':
      manualReset();
      break;
      
    case 's':
      manualParams();
      break;
    
    case 't':
      turbido.state = STATE_TURBIDO_START;
      break;
      
    default:
    {
      Serial.print(F("\r\n# Unknown command: "));
      Serial.write((char) cmd);
      Serial.println();
    }
    break;
  };
}

void manualAnnotate()
{
  int ch;
  
  Serial.print(F("\r\n# NOTE: "));
 
  while (1) {
    while ((ch = Serial.read()) < 0) {
      delay(1);
    }
    
    if (ch == '\n' || ch == '\r') {
      Serial.println();
      break; 
    }
    
    Serial.write(ch);
  } 
}

const unsigned long manualMeasureTime = 500;

void manualMeasure()
{
  Serial.print(F("\r\n"));
  
  while (1) {
    unsigned long tstart = millis();
    struct measure_struct m;
    measure(&m);
    long value = measureV(&m);
    Serial.print(F("# Measure\t"));
    Serial.print(tstart);
    Serial.write('\t');
    Serial.print(value);
    Serial.write("\r\n");

    if (Serial.read() > 0) {
      break; 
    }

    unsigned long tdone = millis();
    if (tdone < tstart + manualMeasureTime) {
      delay((tstart + manualMeasureTime) - tdone); 
    }
  }
}

void manualPump()
{
  Serial.print(F("\r\n# Enter pump duration (sec): "));
  long pumpDurationRequested;
  if (blockingReadLong(&pumpDurationRequested) > 0) {
    Serial.print(F("# Planned pumping time: "));
    Serial.print(pumpDurationRequested);
    Serial.print(F(" sec (any key to interrupt)"));

    unsigned long tstart = millis();
    digitalWrite(motorPin, HIGH);

    unsigned long tend = tstart + pumpDurationRequested * 1000;

    while (millis() < tend) {
      if (Serial.read() > 0) {
        break; 
      }
      delay(1);
    }

    unsigned long tstop = millis();
    digitalWrite(motorPin, LOW);

    unsigned long pumpDurationActual = tstop - tstart;

    snprintf(outbuf, outbuf_len, "\r\n# Pumped %ld.%03ld seconds\r\n", pumpDurationActual / 1000, pumpDurationActual % 1000);
    Serial.write(outbuf);  
  } else {
    Serial.print(F("\r\n# Manual pump cancelled\r\n"));
  }  
}

void manualReset()
{
  turbido.pumpTotalTime = 0;
  Serial.print(F("\r\n# Reset turbidostat pump time"));
}

void manualParams()
{
  struct param_struct p;
  long v;
  readEepromParams(&p);
  Serial.print(F("\r\n# Current settings:\r\n"));
  formatEepromParams(outbuf, outbuf_len, &p);
  Serial.write(outbuf);
  Serial.print(F("# Hit return to leave unchanged\r\n"));

  Serial.print(F("# Enter pump on (high) raw V ("));
  Serial.print(p.pumpOnV);
  Serial.print(F("): "));
  if (blockingReadLong(&v) > 0) {
    p.pumpOnV = v;
  } else {
    Serial.print(F("# (not updated)\r\n"));
  }
  
  Serial.print(F("# Enter pump off (low) raw V ("));
  Serial.print(p.pumpOffV);
  Serial.print(F("): "));
  if (blockingReadLong(&v) > 0) {
    p.pumpOffV = v;
  } else {
    Serial.print(F("# (not updated)\r\n"));
  }  
  
  Serial.print(F("# Enter fill time ("));
  Serial.print(p.volume);
  Serial.print(F("): "));
  if (blockingReadLong(&v) > 0) {
    p.volume = v;
  } else {
    Serial.print(F("# (not updated)\r\n"));
  }  
  
  Serial.print(F("# Enter V @ OD=1.0 ("));
  Serial.print(p.calibration);
  Serial.print(F("): "));
  if (blockingReadLong(&v) > 0) {
    p.calibration = v;
  } else {
    Serial.print(F("# (not updated)\r\n"));
  }  

  Serial.print(F("# Writing new settings to EEPROM\r\n"));
  writeEepromParams(&p);

  Serial.print(F("\r\n# Current settings:\r\n"));
  
  struct param_struct q;
  readEepromParams(&q);
  formatEepromParams(outbuf, outbuf_len, &q);
  Serial.write(outbuf);
}

void turbidoLoop()
{
  long start = millis();
 
  struct measure_struct m;
  measure(&m);
  long v = measureV(&m);
  
  long pumpTime = 0;
  
  if (turbido.pumpState) {
    long now = millis();
    if (turbido.pumpLastTime < 0) {
      turbido.pumpLastTime = now;
    }
    pumpTime = now - turbido.pumpLastTime;
    turbido.pumpLastTime = now;
  }
  
  if (v < turbido.params.pumpOffV) {
    turbido.pumpState = LOW;
    turbido.pumpLastTime = -1; 
  } else if (v > turbido.params.pumpOnV) {
    turbido.pumpState = HIGH;
    if (turbido.pumpLastTime < 0) {
      turbido.pumpLastTime = millis(); 
    }
  }
  
  turbido.pumpTotalTime += pumpTime;
  digitalWrite(motorPin, turbido.pumpState);
  
  long dens = (v * 1000) / turbido.params.calibration;
  long vols = turbido.pumpTotalTime / turbido.params.volume;
  
  snprintf(outbuf, outbuf_len, "%ld\t%ld\t%ld.%03ld\t%ld.%03ld\t%ld.%03ld\r\n", 
           start,
           v, (dens / 1000), (dens % 1000),
           (turbido.pumpTotalTime / 1000), (turbido.pumpTotalTime % 1000), (vols / 1000), (vols % 1000));
  Serial.write(outbuf);
  
  int ch;
  while ((ch = Serial.read()) >= 0) {
    if (ch == 'q') {
      turbido.state = STATE_TURBIDO_STOP;
      while (Serial.read() >= 0) {
        /* DISCARD */ 
      }
    } 
  }
  
  if (turbido.state == STATE_TURBIDO_RUN) {
    long tdelay = tnext * tstep - millis();
    tnext++;
    if (tdelay > 0) {
      delay(tdelay);
    } 
  }
}

long measureV(const struct measure_struct *m)
{
  return ((m->voff) - (m->von)) / ((long) m->nsamp); 
}

const int burnin = 16;
const int nsamp = 128;
const int delayUsec = 520 - 16; // Two bytes of SPI transfer at 1 MHz
// 128 cycles at 1040 microseconds / cycle = 0.133 seconds, covers 7.98 cycles @ 60 Hz
// 256 cycles at 1040 microseconds / cycle = 0.266 seconds, covers 16.0 cycles @ 60 Hz

void measure(struct measure_struct *result)
{
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

const int paramStart = 16;

void readEepromParams(struct param_struct *params)
{
  byte *paramsByte = (byte *) (void *) params;
 
  for (unsigned int i = 0; i < sizeof(struct param_struct); i++) {
    paramsByte[i] = EEPROM.read(paramStart + i);
  } 
}

void writeEepromParams(const struct param_struct *params)
{
  const byte *paramsByte = (const byte *) (const void *) params;
 
  for (unsigned int i = 0; i < sizeof(struct param_struct); i++) {
    EEPROM.write(paramStart + i, paramsByte[i]);
  } 
}

void formatEepromParams(char *buf, unsigned int buflen, const struct param_struct *params)
{
  snprintf(buf, buflen, "# Pump on V: %ld\r\n# Pump off V = %ld\r\n# Volume = %ld sec to fill\r\n# Calibration V = %ld @ OD=1.0\r\n", params->pumpOnV, params->pumpOffV,params->volume, params->calibration);
}

/* Reading integers from Serial */
int blockingReadLong(long *res)
{
  const int buflen = 12;
  char buffer[buflen];
  int bufpos = 0;

  int ch;
  
  do {
    ch = Serial.read();
    if (ch <= 0) {
       delay(1);
    } else if (ch == '\n' || ch == '\r') {
       Serial.println();
       break;
    } else if (ch < '0' || ch > '9') {
       Serial.write('*');
       return -1;
    } else {
       buffer[bufpos] = (char) ch;
       Serial.write(ch);
       bufpos++;
       if (bufpos == (buflen - 1)) {
         break;
       } 
    }
  } while(1);
  
  if (bufpos > 0) {  
    buffer[bufpos] = '\0';
    *res = atol(buffer);
    return 1;
  } else {
    return 0;
  }
}

