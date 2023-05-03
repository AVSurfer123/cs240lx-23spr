#include "quaternion.h"

// Multiply two quaternions and return a copy of the result, prod = L * R
quaternion_t quat_mult (quaternion_t L, quaternion_t R){
    quaternion_t product;
    product.q1 = (L.q1 * R.q1) - (L.q2 * R.q2) - (L.q3 * R.q3) - (L.q4 * R.q4);
    product.q2 = (L.q1 * R.q2) + (L.q2 * R.q1) + (L.q3 * R.q4) - (L.q4 * R.q3);
    product.q3 = (L.q1 * R.q3) - (L.q2 * R.q4) + (L.q3 * R.q1) + (L.q4 * R.q2);
    product.q4 = (L.q1 * R.q4) + (L.q2 * R.q3) - (L.q3 * R.q2) + (L.q4 * R.q1);
    return product;
}

// Multiply a reference of a quaternion by a scalar, q = s*q
static inline void quat_scalar(quaternion_t * q, float scalar){
    q -> q1 *= scalar;
    q -> q2 *= scalar;
    q -> q3 *= scalar;
    q -> q4 *= scalar;
}

// Adds two quaternions together and the sum is the pointer to another quaternion, Sum = L + R
static inline void quat_add(quaternion_t* sum, quaternion_t L, quaternion_t R){
    sum -> q1 = L.q1 + R.q1;
    sum -> q2 = L.q2 + R.q2;
    sum -> q3 = L.q3 + R.q3;
    sum -> q4 = L.q4 + R.q4;
}

// Subtracts two quaternions together and the sum is the pointer to another quaternion, sum = L - R
static inline void quat_sub(quaternion_t* sum, quaternion_t L, quaternion_t R){
    sum -> q1 = L.q1 - R.q1;
    sum -> q2 = L.q2 - R.q2;
    sum -> q3 = L.q3 - R.q3;
    sum -> q4 = L.q4 - R.q4;
}


// the conjugate of a quaternion is it's imaginary component sign changed  q* = [s, -v] if q = [s, v]
static inline quaternion_t quat_conjugate(quaternion_t q){
    q.q2 = -q.q2;
    q.q3 = -q.q3;
    q.q4 = -q.q4;
    return q;
}

// norm of a quaternion is the same as a complex number
// sqrt( q1^2 + q2^2 + q3^2 + q4^2)
// the norm is also the sqrt(q * conjugate(q)), but thats a lot of operations in the quaternion multiplication
static inline float quat_norm (quaternion_t q)
{
    return sqrt(q.q1*q.q1 + q.q2*q.q2 + q.q3*q.q3 +q.q4*q.q4);
}

// Normalizes pointer q by calling quat_Norm(q),
static inline void quat_normalize(quaternion_t* q){
    float norm = quat_norm(*q);
    q -> q1 /= norm;
    q -> q2 /= norm;
    q -> q3 /= norm;
    q -> q4 /= norm;
}

static inline void printQuaternion (quaternion_t q){
    printk("%f %f %f %f\n", q.q1, q.q2, q.q3, q.q4);
}

/*
 returns as pointers, roll pitch and yaw from the quaternion generated in imu_filter
 Assume right hand system
 Roll is about the x axis, represented as phi
 Pitch is about the y axis, represented as theta
 Yaw is about the z axis, represented as psi (trident looking greek symbol)
 */
void eulerAngles(quaternion_t q, double* roll, double* pitch, double* yaw){
    *yaw = atan2((2*q.q2*q.q3 - 2*q.q1*q.q4), (2*q.q1*q.q1 + 2*q.q2*q.q2 -1));  // equation (7)
    *pitch = -asin(2*q.q2*q.q4 + 2*q.q1*q.q3);                                  // equatino (8)
    *roll  = atan2((2*q.q3*q.q4 - 2*q.q1*q.q2), (2*q.q1*q.q1 + 2*q.q4*q.q4 -1));
    
    *yaw *= (180.0f / M_PI);
    *pitch *= (180.0f / M_PI);
    *roll *= (180.0f / M_PI);
}
