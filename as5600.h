/*
 * File: dwm_linux_AS5600.c
 * Project: dwm_linux_as5600
 * -----
 * This source code is released under BSD-3 license.
 * Adapted for standard Linux environment using /dev/i2c interface.
 * -----
 * Copyright 2025 Adapted by ChatGPT
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <math.h>

#define AS5600_DEFAULT_ADDRESS 0x36
#define AS5600_RW_MAX 2
#define REG 1
#define BYTE 8
#define AS5600_MAX_ANGLE 4096
#define BURN_ANGLE 0x80
#define BURN_SETTING 0x40
#define SQUARE(x) ((x)*(x))

// AS5600 registers and lengths
typedef enum {
    ZMCO = 0x00, ZPOS = 0x01, MPOS = 0x03, MANG = 0x05,
    CONF = 0x07, RAW_ANGLE = 0x0c, ANGLE = 0x0e,
    STATUS = 0x0b, AGC = 0x1a, MAGNITUDE = 0x1b,
    BURN = 0xff
} as5600_reg_t;

typedef enum {
    ZMCO_LEN = 1, ZPOS_LEN = 2, MPOS_LEN = 2, MANG_LEN = 2,
    CONF_LEN = 2, RAW_ANGLE_LEN = 2, ANGLE_LEN = 2,
    STATUS_LEN = 1, AGC_LEN = 1, MAGNITUDE_LEN = 2, BURN_LEN = 1
} as5600_data_len_t;

// AS5600 context
typedef struct {
    int fd; // file descriptor for I2C device
} as5600_t;

/**
 * Initialize AS5600 on given I2C bus (e.g. "/dev/i2c-1").
 * Returns 0 on success, -1 on failure.
 */
int as5600_init(const char *i2c_bus, as5600_t *dev) {
    dev->fd = open(i2c_bus, O_RDWR);
    if (dev->fd < 0) {
        perror("Opening I2C bus");
        return -1;
    }
    if (ioctl(dev->fd, I2C_SLAVE, AS5600_DEFAULT_ADDRESS) < 0) {
        perror("Setting I2C_SLAVE address");
        close(dev->fd);
        return -1;
    }
    return 0;
}

/**
 * Generic I2C read
 */
uint16_t as5600_read(as5600_t *dev, uint8_t reg, uint8_t len) {
    uint8_t buff[AS5600_RW_MAX] = {0};
    if (write(dev->fd, &reg, 1) != 1) {
        perror("I2C write register");
        return 0;
    }
    if (read(dev->fd, buff, len) != len) {
        perror("I2C read data");
        return 0;
    }
    if (len == 1)
        return buff[0];
    return (buff[0] << BYTE) | buff[1];
}

/**
 * Generic I2C write
 */
void as5600_write(as5600_t *dev, uint8_t reg, uint16_t val, uint8_t len) {
    uint8_t buff[REG + AS5600_RW_MAX] = {0};
    buff[0] = reg;
    if (len == 1) {
        buff[1] = val & 0xFF;
    } else {
        buff[1] = (val >> BYTE) & 0xFF;
        buff[2] = val & 0xFF;
    }
    if (write(dev->fd, buff, len + REG) < 0) {
        perror("I2C write data");
    }
}

// High-level API functions
uint8_t as5600_read_zmco(as5600_t *dev) { return as5600_read(dev, ZMCO, ZMCO_LEN); }
uint16_t as5600_read_zpos(as5600_t *dev) { return as5600_read(dev, ZPOS, ZPOS_LEN); }
void as5600_write_zpos(as5600_t *dev, uint16_t angl) { as5600_write(dev, ZPOS, angl, ZPOS_LEN); }
uint16_t as5600_read_mpos(as5600_t *dev) { return as5600_read(dev, MPOS, MPOS_LEN); }
void as5600_write_mpos(as5600_t *dev, uint16_t angl) { as5600_write(dev, MPOS, angl, MPOS_LEN); }
uint16_t as5600_read_mang(as5600_t *dev) { return as5600_read(dev, MANG, MANG_LEN); }
void as5600_write_mang(as5600_t *dev, uint16_t angl) { as5600_write(dev, MANG, angl, MANG_LEN); }
uint16_t as5600_read_raw_angl(as5600_t *dev) { return as5600_read(dev, RAW_ANGLE, RAW_ANGLE_LEN); }
uint16_t as5600_read_angl(as5600_t *dev) { return as5600_read(dev, ANGLE, ANGLE_LEN); }
uint8_t as5600_read_status(as5600_t *dev) { return as5600_read(dev, STATUS, STATUS_LEN); }
uint8_t as5600_read_agc(as5600_t *dev) { return as5600_read(dev, AGC, AGC_LEN); }
int16_t as5600_read_magnitude(as5600_t *dev) { return as5600_read(dev, MAGNITUDE, MAGNITUDE_LEN); }
void as5600_burn_angle(as5600_t *dev) { as5600_write(dev, BURN, BURN_ANGLE, BURN_LEN); }
void as5600_burn_setting(as5600_t *dev) { as5600_write(dev, BURN, BURN_SETTING, BURN_LEN); }

// Conversion helpers
uint16_t as5600_mang_to_mpos(uint16_t zpos, uint16_t mang) {
    return (zpos + mang) % AS5600_MAX_ANGLE;
}
uint16_t as5600_angl_to_degr(uint16_t angl, uint16_t zpos, uint16_t mpos) {
    return (uint32_t)angl * (mpos - zpos) / (SQUARE(AS5600_MAX_ANGLE) / 360);
}
float as5600_angl_to_degr_float(uint16_t angl, uint16_t zpos, uint16_t mpos) {
    return (uint32_t)angl * (mpos - zpos) / (SQUARE(AS5600_MAX_ANGLE) / 360.0f);
}
uint16_t as5600_float_degrees_to_angl(float degr) {
    degr = fmodf(degr, 360.0f);
    return (uint16_t)(AS5600_MAX_ANGLE * degr / 360.0f);
}
uint16_t as5600_degrees_to_angl(uint16_t degr) {
    degr %= 360;
    return (uint16_t)(AS5600_MAX_ANGLE * degr / 360);
}
int8_t as5600_status_to_scale(uint8_t status) {
    const uint8_t MD = 0x20, ML = 0x10, MH = 0x40;
    if (status & MH) return (status & MD) ? 1 : 2;
    if (status & ML) return (status & MD) ? -1 : -2;
    return 0;
}
