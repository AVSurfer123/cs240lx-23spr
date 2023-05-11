#pragma once

#include "rpi.h"
#include "rpi-math.h"


#define GYRO_MEAN_ERROR M_PI * (5.0f / 180.0f)
#define BETA sqrt(3.0f/4.0f) * GYRO_MEAN_ERROR

typedef struct {
    double q1; // w
    double q2; // x
    double q3; // y
    double q4; // z
} quaternion_t;

typedef struct {
    double x, y, z;
} angles_t;


static inline void printQuat(const char *msg, quaternion_t q) {
    printk("%s: %f %f %f %f\n", msg, q.q1, q.q2, q.q3, q.q4);
}

static inline void angles_print(const char *msg, angles_t xyz) {
    output("%s (x=%f,y=%f,z=%f)\n", msg, xyz.x, xyz.y, xyz.z);
}

quaternion_t quat_mult(quaternion_t L, quaternion_t R);
quaternion_t quat_scalar(quaternion_t q, double scalar);
quaternion_t quat_add(quaternion_t L, quaternion_t R);
quaternion_t quat_sub(quaternion_t L, quaternion_t R);
quaternion_t quat_conjugate(quaternion_t q) ;
double quat_norm(quaternion_t q);
void quat_normalize(quaternion_t *q);

angles_t eulerAngles(quaternion_t q);
double imu_filter(angles_t accel, angles_t gyro, double last_time);
