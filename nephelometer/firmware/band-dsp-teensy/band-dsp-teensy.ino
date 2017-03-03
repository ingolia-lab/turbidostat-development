#include "nephelometer.h"
#include "supervisor.h"
 
Supervisor *sv;

void setup() {
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  pinMode(13, OUTPUT);
  pinMode(11, OUTPUT);

  SPI.setMOSI(11);
  SPI.setSCK(13);

  pinMode(A10, INPUT);
  pinMode(A11, INPUT);
  
  Serial.begin(9600);
  SPI.begin();

  Serial.print("Preparing...");
  
  for (int i = 9; i >= 0; i--) {
    delay(500);
    Serial.print(i);
    Serial.write(' ');
  }
  Serial.println();

  sv = new Supervisor();

  sv->begin();
}

void loop() {
  sv->loop();
}

