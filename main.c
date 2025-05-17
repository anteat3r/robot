#include "wiringPi.h"
#include <stdio.h>

int main() {
  wiringPiSetupGpio();
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);
  printf("HIGH\n");
  delay(1000);
  digitalWrite(23, LOW);
  printf("LOW\n");
}
