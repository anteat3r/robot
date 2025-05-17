#include "wiringPi.h"
#include "i2cp.h"
#include "as5600.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpu6050.h"

int main() {
  wiringPiSetupGpio();
  PCA9685 pca = pca_new("/dev/i2c-1", 0x40);
  pca_set_pwm_freq(&pca, 50);

  as5600_t sensor;
  if (as5600_init("/dev/i2c-1", &sensor) != 0) {
    perror("as5600 init");
    exit(1);
  }

  mpu6050_t accels;
  if (mpu6050_init("/dev/i2c-1", &accels) != 0) {
    perror("mpu6050 init");
    exit(1);
  }

  while (1) {
    // double ms = .5 + (sin((double)micros() / 200000.) + 1.) / 2. * 2.;
    // pca_set_pwm_ms(pca, 0, ms);

    // uint16_t angle = as5600_read_angl(&sensor);
    // printf("%d\n", (int)angle);

    float ax, ay, az;
    mpu6050_get_gyro(&accels, &ax, &ay, &az);
    printf("%f %f %f\n", ax, ay, az);
  }
}
