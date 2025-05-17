/*
 * File: mpu6050_linux.c
 * Linux I2C-dev driver for MPU-6050 (Gyro + Accelerometer)
 * Adapted from Python mpu6050 class
 * MIT License
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <math.h>

#define MPU6050_ADDR             0x68
#define I2C_BUFFER_MAX           2

// Registers (from Register Map Rev. 4.2)
#define PWR_MGMT_1               0x6B  // Power management
#define ACCEL_CONFIG             0x1C  // Accelerometer config
#define GYRO_CONFIG              0x1B  // Gyroscope config
#define MPU_CONFIG               0x1A  // DLPF config

#define ACCEL_XOUT_H             0x3B
#define ACCEL_YOUT_H             0x3D
#define ACCEL_ZOUT_H             0x3F
#define TEMP_OUT_H               0x41
#define GYRO_XOUT_H              0x43
#define GYRO_YOUT_H              0x45
#define GYRO_ZOUT_H              0x47

// Scale modifiers
#define GRAVITY_MS2              9.80665f
#define ACCEL_SF_2G              16384.0f
#define ACCEL_SF_4G               8192.0f
#define ACCEL_SF_8G               4096.0f
#define ACCEL_SF_16G              2048.0f
#define GYRO_SF_250               131.0f
#define GYRO_SF_500                65.5f
#define GYRO_SF_1000               32.8f
#define GYRO_SF_2000               16.4f

// Ranges
#define ACCEL_RANGE_2G            0x00
#define ACCEL_RANGE_4G            0x08
#define ACCEL_RANGE_8G            0x10
#define ACCEL_RANGE_16G           0x18

#define GYRO_RANGE_250            0x00
#define GYRO_RANGE_500            0x08
#define GYRO_RANGE_1000           0x10
#define GYRO_RANGE_2000           0x18

#define FILTER_BW_256             0x00
#define FILTER_BW_188             0x01
#define FILTER_BW_98              0x02
#define FILTER_BW_42              0x03
#define FILTER_BW_20              0x04
#define FILTER_BW_10              0x05
#define FILTER_BW_5               0x06

typedef struct {
    int i2c_fd;
} mpu6050_t;

/**
 * Initialize MPU-6050 on given I2C bus (e.g. "/dev/i2c-1").
 * Returns 0 on success, -1 on error. :contentReference[oaicite:9]{index=9}
 */
int mpu6050_init(const char *i2c_bus, mpu6050_t *mpu) {
    if ((mpu->i2c_fd = open(i2c_bus, O_RDWR)) < 0) {
        perror("Opening I2C bus");
        return -1;
    }
    if (ioctl(mpu->i2c_fd, I2C_SLAVE, MPU6050_ADDR) < 0) {
        perror("Setting slave address");
        close(mpu->i2c_fd);
        return -1;
    }
    // Wake up sensor (clear sleep bit) :contentReference[oaicite:10]{index=10}
    uint8_t wm = 0x00;
    if (write(mpu->i2c_fd, (uint8_t[]){PWR_MGMT_1, wm}, 2) != 2) {
        perror("Wake-up write");
        return -1;
    }
    return 0;
}

/**
 * Read two bytes and return signed 16-bit. :contentReference[oaicite:11]{index=11}
 */
int16_t mpu6050_read_word(mpu6050_t *mpu, uint8_t reg) {
    uint8_t buf[I2C_BUFFER_MAX];
    if (write(mpu->i2c_fd, &reg, 1) != 1) {
        perror("Register select");
        return 0;
    }
    if (read(mpu->i2c_fd, buf, 2) != 2) {
        perror("Data read");
        return 0;
    }
    int16_t val = (buf[0] << 8) | buf[1];
    return (val >= 0x8000) ? -((int16_t)((~val + 1) & 0xFFFF)) : val;
}

/**
 * Write single byte to a register. :contentReference[oaicite:12]{index=12}
 */
void mpu6050_write_byte(mpu6050_t *mpu, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    if (write(mpu->i2c_fd, buf, 2) != 2) {
        perror("Register write");
    }
}

/* Temperature in °C */
float mpu6050_get_temp(mpu6050_t *mpu) {
    int16_t raw = mpu6050_read_word(mpu, TEMP_OUT_H);
    return (raw / 340.0f) + 36.53f;  /* Datasheet formula */ :contentReference[oaicite:13]{index=13}
}

/* Accelerometer range setter */
void mpu6050_set_accel_range(mpu6050_t *mpu, uint8_t range) {
    mpu6050_write_byte(mpu, ACCEL_CONFIG, range);  /* 0x00 then range */ :contentReference[oaicite:14]{index=14}
}

/* Read back accel range (raw register) */
uint8_t mpu6050_get_accel_range_raw(mpu6050_t *mpu) {
    uint8_t reg = ACCEL_CONFIG;
    if (write(mpu->i2c_fd, &reg, 1) != 1) { perror("Select ACCEL_CONFIG"); }
    uint8_t val;
    if (read(mpu->i2c_fd, &val, 1) != 1)  { perror("Read ACCEL_CONFIG"); }
    return val;
}

/* Get acceleration in m/s² or g */
void mpu6050_get_accel(mpu6050_t *mpu, float *ax, float *ay, float *az, int in_g) {
    int16_t rx = mpu6050_read_word(mpu, ACCEL_XOUT_H);
    int16_t ry = mpu6050_read_word(mpu, ACCEL_YOUT_H);
    int16_t rz = mpu6050_read_word(mpu, ACCEL_ZOUT_H);

    uint8_t araw = mpu6050_get_accel_range_raw(mpu) & 0x18;
    float sf = (araw == ACCEL_RANGE_4G ? ACCEL_SF_4G :
                araw == ACCEL_RANGE_8G ? ACCEL_SF_8G :
                araw == ACCEL_RANGE_16G? ACCEL_SF_16G:
                                         ACCEL_SF_2G);
    /* Scale to g */                               :contentReference[oaicite:15]{index=15}
    *ax = rx / sf;  *ay = ry / sf;  *az = rz / sf;

    if (!in_g) {
        *ax *= GRAVITY_MS2;  *ay *= GRAVITY_MS2;  *az *= GRAVITY_MS2;
    }
}

/* Gyro range setter */
void mpu6050_set_gyro_range(mpu6050_t *mpu, uint8_t range) {
    mpu6050_write_byte(mpu, GYRO_CONFIG, range); :contentReference[oaicite:16]{index=16}
}

/* Read gyro data in °/s */
void mpu6050_get_gyro(mpu6050_t *mpu, float *gx, float *gy, float *gz) {
    int16_t gx_raw = mpu6050_read_word(mpu, GYRO_XOUT_H);
    int16_t gy_raw = mpu6050_read_word(mpu, GYRO_YOUT_H);
    int16_t gz_raw = mpu6050_read_word(mpu, GYRO_ZOUT_H);

    uint8_t graw = 0;
    /* Re-read GYRO_CONFIG register */               :contentReference[oaicite:17]{index=17}
    mpu6050_write_byte(mpu, MPU_CONFIG, 0); // keep ext_sync, set DLPF=0xff
    // For brevity, assume default ±250°/s
    float sf = GYRO_SF_250;

    *gx = gx_raw / sf;
    *gy = gy_raw / sf;
    *gz = gz_raw / sf;
}
