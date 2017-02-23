#include "supervisor.h"
 
Supervisor sv;

void setup() {
  Serial.begin(9600);
  sv.begin();
}

void loop() {
  sv.loop();
}

