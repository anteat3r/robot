#include "wiringPi.h"

int main() {
  wiringPiSetupGpio();
  digitalWrite(23, HIGH);
  delay(1000);
  digitalWrite(23, LOW);
}
