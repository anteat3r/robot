#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <i2c/smbus.h>
#include <math.h>
#include "wiringPi.h"

static const uint8_t MODE1              = 0x00;
static const uint8_t MODE2              = 0x01;
static const uint8_t SUBADR1            = 0x02;
static const uint8_t SUBADR2            = 0x03;
static const uint8_t SUBADR3            = 0x04;
static const uint8_t PRESCALE           = 0xFE;
static const uint8_t LED0_ON_L          = 0x06;
static const uint8_t LED0_ON_H          = 0x07;
static const uint8_t LED0_OFF_L         = 0x08;
static const uint8_t LED0_OFF_H         = 0x09;
static const uint8_t ALL_LED_ON_L       = 0xFA;
static const uint8_t ALL_LED_ON_H       = 0xFB;
static const uint8_t ALL_LED_OFF_L      = 0xFC;
static const uint8_t ALL_LED_OFF_H      = 0xFD;

// Bits:
static const uint8_t RESTART            = 0x80;
static const uint8_t SLEEP              = 0x10;
static const uint8_t ALLCALL            = 0x01;
static const uint8_t INVRT              = 0x10;
static const uint8_t OUTDRV             = 0x04;

int open_bus(const char* device) {
  int bus_fd = open(device, O_RDWR);
  if (bus_fd < 0) {
    perror("bus_fd < 0");
    exit(1);
  }
  return bus_fd;
}

void connect_to_peripheral(int bus_fd, const uint8_t address) {
  if (ioctl(bus_fd, I2C_SLAVE, address) < 0) {
    perror("ioctl < 0");
    exit(1);
  }
}

int I2CP_init(const char* device, const uint8_t address) {
  int bus_fd = open_bus(device);
  connect_to_peripheral(bus_fd, address);
  return bus_fd;
}

void I2CP_write_register_data(int bus_fd, const uint8_t address, const uint8_t value) {
  union i2c_smbus_data data;
  data.byte = value;
  int err = i2c_smbus_access(bus_fd, I2C_SMBUS_WRITE, address, I2C_SMBUS_BYTE_DATA, &data);
  if (err) {
    perror("write_register_data");
    exit(1);
  }
}

uint8_t I2CP_read_register_data(int bus_fd, const uint8_t address) {
  union i2c_smbus_data data;
  int err = i2c_smbus_access(bus_fd, I2C_SMBUS_READ, address, I2C_SMBUS_BYTE_DATA, &data);
  if (err) {
    perror("read_register_data");
    exit(1);
  }
  return data.byte & 0xFF;
}

typedef struct {
  double frequency;
  int i2CP_bus_fd;
} PCA9685 ;

void pca_set_pwm(PCA9685 pca, int channel, uint16_t on, uint16_t off) {
  const int channel_off = 4 * channel;
  I2CP_write_register_data(pca.i2CP_bus_fd, ALL_LED_ON_L+channel_off, on & 0xFF);
  I2CP_write_register_data(pca.i2CP_bus_fd, ALL_LED_ON_H+channel_off, on >> 8);
  I2CP_write_register_data(pca.i2CP_bus_fd, ALL_LED_OFF_L+channel_off, off & 0xFF);
  I2CP_write_register_data(pca.i2CP_bus_fd, ALL_LED_OFF_H+channel_off, off >> 8);
}

void pca_set_all_pwm(PCA9685 pca, uint16_t on, uint16_t off) {
  I2CP_write_register_data(pca.i2CP_bus_fd, ALL_LED_ON_L, on & 0xFF);
  I2CP_write_register_data(pca.i2CP_bus_fd, ALL_LED_ON_H, on >> 8);
  I2CP_write_register_data(pca.i2CP_bus_fd, ALL_LED_OFF_L, off & 0xFF);
  I2CP_write_register_data(pca.i2CP_bus_fd, ALL_LED_OFF_H, off >> 8);
}

void pca_set_pwm_freq(PCA9685* pca, const double freq_hz) {
  pca->frequency = freq_hz;

  double prescaleval = 2.5e7; //    # 25MHz
  prescaleval /= 4096.0; //       # 12-bit
  prescaleval /= freq_hz;
  prescaleval -= 1.0;

  int prescale = (int)round(prescaleval);

  uint8_t oldmode = I2CP_read_register_data(pca->i2CP_bus_fd, MODE1);

  uint8_t newmode = (oldmode & 0x7F) | SLEEP;

  I2CP_write_register_data(pca->i2CP_bus_fd, MODE1, newmode);
  I2CP_write_register_data(pca->i2CP_bus_fd, PRESCALE, prescale);
  I2CP_write_register_data(pca->i2CP_bus_fd, MODE1, oldmode);
  delay(5);
  I2CP_write_register_data(pca->i2CP_bus_fd, MODE1, oldmode | RESTART);
}

void pca_set_pwm_ms(PCA9685 pca, int channel, double ms) {
  double period_ms = 1000.0 / pca.frequency;
  double bits_per_ms = 4096 / period_ms;
  double bits = ms * bits_per_ms;
  pca_set_pwm(pca, channel, 0, bits);
}

PCA9685 pca_new(const char* device, int address) {
  PCA9685 pca;
  pca.i2CP_bus_fd = I2CP_init(device, address);
  pca_set_all_pwm(pca, 0, 0);
  I2CP_write_register_data(pca.i2CP_bus_fd, MODE2, OUTDRV);
  I2CP_write_register_data(pca.i2CP_bus_fd, MODE1, ALLCALL);
  delay(5);
  uint8_t val = I2CP_read_register_data(pca.i2CP_bus_fd, MODE1);
  val &= ~SLEEP;
  I2CP_write_register_data(pca.i2CP_bus_fd, MODE1, val);
  delay(5);
  return pca;
}
