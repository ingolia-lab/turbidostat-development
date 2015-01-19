#include <SPI.h>

const int motorPin = 13;
const int osc10Pin = 4;
const int osc5Pin = 5;
const int switch1 = 3;
const int switch2 = 2;

const int adcChipSel = 20; // A2	D20 	PF5	ADC5

const long timestep = 100;

void setup()
{
   pinMode(motorPin, OUTPUT);
   digitalWrite(motorPin, LOW);
   
   pinMode(osc10Pin, INPUT);
   
   pinMode(osc5Pin, INPUT);
   
   pinMode(switch1, INPUT);
   
   pinMode(switch2, INPUT);
   
   pinMode(adcChipSel, OUTPUT);
   digitalWrite(adcChipSel, HIGH);
   
   SPI.setClockDivider( SPI_CLOCK_DIV16 );
   SPI.setBitOrder(MSBFIRST);
   SPI.setDataMode(SPI_MODE0);
   SPI.begin();
   
   Serial.begin(9600);
   Serial.print(F("band-psd-micro-standalone_test\r\n"));
}



void loop()
{
  long now = millis();
  long next = now + timestep;

  int motorState = (((now / 1000) % 10) > 4) ? HIGH : LOW;
  digitalWrite(motorPin, motorState);
  
  int measure = readAdc();
  
  int osc10State = digitalRead(osc10Pin);
  int osc5State = digitalRead(osc5Pin);
  
  Serial.print(now);
  Serial.write('\t');
  Serial.print(motorState);
  Serial.write('\t');
  Serial.print(measure);
  Serial.write('\t');
  Serial.print(osc10State);
  Serial.write('\t');
  Serial.print(osc5State);
  Serial.println();
  
  long dt = next - millis();
  if (dt > 0 && dt < timestep) { delay (dt); }
}

int readAdc(void) {
  int adcval = 0;
  digitalWrite(adcChipSel, LOW);
  int b1 = SPI.transfer(0);
  int b2 = SPI.transfer(0);
  digitalWrite(adcChipSel, HIGH);

  int sign = b1 & B00010000;
  int hi =   b1 & B00001111;
  int lo =   b2;
  
  int reading = hi * 256 + lo;
  if (sign) { reading = (4096 - reading) * -1; }
  
  Serial.print(b1);
  Serial.write('\t');
  Serial.print(b2);
  Serial.write('\t');
  
  return reading;
}
