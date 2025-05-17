#include "wiringPi.h"
#include "i2cp.h"
#include <math.h>
#include <stdio.h>

int main() {
  wiringPiSetupGpio();
  PCA9685 pca = pca_new("/dev/i2c-1", 0x40);
  pca_set_pwm_freq(&pca, 50);

  while (1) {
    double ms = 1 + (sin((double)micros() / 200000.) + 1.) / 2. * 2.;
    pca_set_pwm_ms(pca, 0, ms);
    printf("%f        \r", ms);
    // delay(10);
  }
}
