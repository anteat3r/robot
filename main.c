#include "wiringPi.h"

int main() {
  wiringPiSetupGpio();
  digitalWrite(2, HIGH);
  delay(1000);
  digitalWrite(2, LOW);
}
