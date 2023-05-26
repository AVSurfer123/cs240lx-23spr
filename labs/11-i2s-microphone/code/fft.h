#ifndef FFT_H
#define FFT_H

#include "rpi.h"
#include "rpi-math.h"

// int16 * int16 -> int32 multiply -- specific to arm_none_eabi
inline int32_t fft_fixed_mul(int16_t a, int16_t b) {
    int32_t result;
    asm("smulbb %0, %1, %2" : "=r" (result) : "r" (a), "r" (b));
    return result;
}

// converts uint32 centered at INT_MAX into int16 (Q.15) centered at 0
inline int16_t to_q15(uint32_t x) {
    // uint32_t raw = x >> 14; // Upper 18 bits
    int32_t a = x - (1 << 31);
    return a >> 10; // doing 10 instead of 16
}

// multiplies Q.15 * Q.15 into Q.15
inline int16_t fft_fixed_mul_q15(int16_t a, int16_t b) {
    int32_t c = fft_fixed_mul(a, b);
    // save the most significant bit that's lost (round up if set)
    int32_t round = (c >> 14) & 1;
    return (c >> 15) + round;
}

int32_t fft_fixed_cfft(int16_t *real, int16_t *imag, int16_t log2_len, unsigned inverse);
int32_t fft_fixed_rfft(int16_t *data, int32_t log2_len, unsigned inverse);


void fft(int N, double* x, double* y);
inline double convert_sample(uint32_t x) { 
    // int32_t s = *(int32_t*) &x;
    int32_t s = (int32_t) x;
    s = s >> 10;
    double a = (double) s;
    a = a / (1 << 17);
    printk("testing div %f %f\n", (double) s, a);
    return a;
}

#endif // FFT_H