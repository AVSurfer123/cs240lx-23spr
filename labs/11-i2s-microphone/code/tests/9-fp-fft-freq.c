/*
 * Print out fundamental frequency heard
 */

#include "rpi.h"
#include "fft.h"
#include "i2s.h"

#define FFT_LEN 1024
#define LOG2_FFT_LEN 10
#define FS 44100
// attempt to reject harmonics. change this if you're
// seeing multiples of the fundamental frequency 
#define MAX_THRESH_FACTOR 8

void notmain(void) {
    enable_cache();
    i2s_init(44100);

    double real[FFT_LEN] = {0};
    double imag[FFT_LEN] = {0};

    uint32_t start= timer_get_usec();
    while (timer_get_usec() - start < 10e6) {

        // real samples: set imaginary part to 0
        for (int i = 0; i < FFT_LEN; i++) {
            uint32_t x = i2s_read_sample();
            real[i] = convert_sample(x);
            // printk("Sample %d: %d %f\n", i, x, real[i]);
            imag[i] = 0;
        }

        fft(FFT_LEN, real, imag);

        double data_max = 0;
        double data_max_idx = 0;

        for (int i = 0; i < FFT_LEN; i++) {
            double mag = sqrt(real[i] * real[i] + imag[i] * imag[i]);
            // attempt to reject harmonics by requiring higher frequencies to be some factor larger
            if (mag > data_max * MAX_THRESH_FACTOR) {
                data_max = mag;
                data_max_idx = i;
            }
        }

        double freq = data_max_idx * FS / FFT_LEN;

        printk("%f\n", freq);

    }

    clean_reboot();

}