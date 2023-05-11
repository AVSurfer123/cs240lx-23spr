#include "quaternion.h"

// Multiply two quaternions and return a copy of the result, prod = L * R
quaternion_t quat_mult(quaternion_t L, quaternion_t R) {
    quaternion_t product;
    product.q1 = (L.q1 * R.q1) - (L.q2 * R.q2) - (L.q3 * R.q3) - (L.q4 * R.q4);
    product.q2 = (L.q1 * R.q2) + (L.q2 * R.q1) + (L.q3 * R.q4) - (L.q4 * R.q3);
    product.q3 = (L.q1 * R.q3) - (L.q2 * R.q4) + (L.q3 * R.q1) + (L.q4 * R.q2);
    product.q4 = (L.q1 * R.q4) + (L.q2 * R.q3) - (L.q3 * R.q2) + (L.q4 * R.q1);
    return product;
}

// Multiply a reference of a quaternion by a scalar, q = s*q
inline quaternion_t quat_scalar(quaternion_t q, double scalar) {
    q.q1 *= scalar;
    q.q2 *= scalar;
    q.q3 *= scalar;
    q.q4 *= scalar;
    return q;
}

// Adds two quaternions together and the sum is the pointer to another quaternion, Sum = L + R
inline quaternion_t quat_add(quaternion_t L, quaternion_t R) {
    quaternion_t sum;
    sum.q1 = L.q1 + R.q1;
    sum.q2 = L.q2 + R.q2;
    sum.q3 = L.q3 + R.q3;
    sum.q4 = L.q4 + R.q4;
    return sum;
}

// Subtracts two quaternions together and the sum is the pointer to another quaternion, sum = L - R
inline quaternion_t quat_sub(quaternion_t L, quaternion_t R) {
    quaternion_t sum;
    sum.q1 = L.q1 - R.q1;
    sum.q2 = L.q2 - R.q2;
    sum.q3 = L.q3 - R.q3;
    sum.q4 = L.q4 - R.q4;
    return sum;
}

// the conjugate of a quaternion is it's imaginary component sign changed  q* = [s, -v] if q = [s, v]
inline quaternion_t quat_conjugate(quaternion_t q) {
    q.q2 = -q.q2;
    q.q3 = -q.q3;
    q.q4 = -q.q4;
    return q;
}

// norm of a quaternion is the same as a complex number
// sqrt( q1^2 + q2^2 + q3^2 + q4^2)
// the norm is also the sqrt(q * conjugate(q)), but thats a lot of operations in the quaternion multiplication
inline double quat_norm(quaternion_t q) {
    return sqrt(q.q1 * q.q1 + q.q2 * q.q2 + q.q3 * q.q3 + q.q4 * q.q4);
}

// Normalizes pointer q by calling quat_Norm(q),
inline void quat_normalize(quaternion_t *q) {
    double norm = quat_norm(*q);
    q->q1 /= norm;
    q->q2 /= norm;
    q->q3 /= norm;
    q->q4 /= norm;
}

/*
 returns as pointers, roll pitch and yaw from the quaternion generated in imu_filter
 Assume right hand system
 Roll is about the x axis, represented as phi
 Pitch is about the y axis, represented as theta
 Yaw is about the z axis, represented as psi (trident looking greek symbol)
 */
angles_t eulerAngles(quaternion_t q) {
    double yaw = atan2((2 * q.q2 * q.q3 - 2 * q.q1 * q.q4), (2 * q.q1 * q.q1 + 2 * q.q2 * q.q2 - 1)); // equation (7)
    double pitch = -asin(2 * q.q2 * q.q4 + 2 * q.q1 * q.q3);                                          // equatino (8)
    double roll = atan2((2 * q.q3 * q.q4 - 2 * q.q1 * q.q2), (2 * q.q1 * q.q1 + 2 * q.q4 * q.q4 - 1));

    angles_t angles;
    angles.x = roll * 180 / M_PI;
    angles.y = pitch * 180 / M_PI;
    angles.z = yaw * 180 / M_PI;
    return angles;
}

quaternion_t q_est = {1, 0, 0, 0};

// IMU consists of a Gyroscope plus Accelerometer sensor fusion
// accel should be in gs, gyro in rad/s
double imu_filter(angles_t accel, angles_t gyro, double last_time) {
    quaternion_t omega = {0, gyro.x, gyro.y, gyro.z};
    quaternion_t omega_dot = quat_mult(quat_scalar(q_est, .5), omega);

    quaternion_t a = {0, accel.x, accel.y, accel.z};
    quat_normalize(&a);

    double F_g[3] = {
        2*(q_est.q2*q_est.q4 - q_est.q1*q_est.q3) - a.q2,
        2*(q_est.q1*q_est.q2 + q_est.q3*q_est.q4) - a.q3,
        2*(.5 - q_est.q2*q_est.q2 - q_est.q3*q_est.q3) - a.q4
    };
    double J_g[3][4] = {
        {-2*q_est.q3, 2*q_est.q4, -2*q_est.q1, 2*q_est.q2},
        {2*q_est.q2, 1*q_est.q1, 2*q_est.q4, 2*q_est.q3},
        {0, -4*q_est.q2, -4*q_est.q3, 0}
    }; 

    quaternion_t gradient;
    gradient.q1 = J_g[0][0] * F_g[0] + J_g[1][0] * F_g[1] + J_g[2][0] * F_g[2];
    gradient.q2 = J_g[0][1] * F_g[0] + J_g[1][1] * F_g[1] + J_g[2][1] * F_g[2];
    gradient.q3 = J_g[0][2] * F_g[0] + J_g[1][2] * F_g[1] + J_g[2][2] * F_g[2];
    gradient.q4 = J_g[0][3] * F_g[0] + J_g[1][3] * F_g[1] + J_g[2][3] * F_g[2];
    quat_normalize(&gradient);

    gradient = quat_scalar(gradient, BETA);
    quaternion_t qdot = quat_sub(omega_dot, gradient);
    double cur_time = timer_get_usec() / 1000000.0;
    q_est = quat_add(q_est, quat_scalar(qdot, cur_time - last_time));
    quat_normalize(&q_est);
    // printk("Time delta: %f\n", cur_time - last_time);
    return cur_time;
}
