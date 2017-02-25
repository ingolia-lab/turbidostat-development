#include "supervisor.h"
 
Supervisor *sv;

void setup() {
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

