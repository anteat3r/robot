#include "wiringPi.h"
#include "i2cp.h"

int main() {
  wiringPiSetupGpio();
  PCA9685 pca = pca_new("/dev/i2c-1", 0x40);
  pca_set_pwm_freq(&pca, 50);
  while (1) {
    pca_set_pwm_ms(pca, 0, 3);
    delay(500);
    pca_set_pwm_ms(pca, 0, 1);
    delay(500);
  }
}
