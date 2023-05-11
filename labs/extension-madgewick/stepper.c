#include "stepper.h"
#include "bit-support.h"

#define DIR 20
#define STEP 21

#define MS1 6
#define MS2 5
#define MS3 7

#define MAX_STEP_FREQ 14285 / 4

void stepper_init() {
    gpio_set_output(DIR);
    gpio_set_output(STEP);
    gpio_set_output(MS1);
    gpio_set_output(MS2);
    gpio_set_output(MS3);

    // Choose microstep mode
    gpio_set_on(MS1);
    gpio_set_off(MS2);
    gpio_set_off(MS3);
}

void run_stepper(double power) {
    if (power > 0) {
        gpio_set_on(DIR);
    }
    else {
        gpio_set_off(DIR);
        power = -power;
    }

    int freq = MAX_STEP_FREQ * power;
    int delay_time = 1000000.0 / freq;
    gpio_set_on(STEP);
    delay_us(delay_time);
    gpio_set_off(STEP);
    delay_us(delay_time);
}

void step(int resolution, double speed) {
    int order;
    switch(resolution) {
        case 1:
            order = 0b000;
            break;
        case 2:
            order = 0b100;
            break;
        case 4:
            order = 0b010;
            break;
        case 8:
            order = 0b110;
            break;
        case 16:
            order = 0b111;
            break;
        default:
            panic("Provided invalid stepper resolution %d\n", resolution);
    };
    gpio_write(MS1, bit_get(order, 2));
    gpio_write(MS2, bit_get(order, 1));
    gpio_write(MS3, bit_get(order, 0));
    printk("MS %d %d %d %d\n", gpio_read(MS1), gpio_read(MS2), gpio_read(MS3));

    if (speed > 0) {
        gpio_set_on(DIR);
    }
    else {
        gpio_set_off(DIR);
    }

    for (int i = 0; i < resolution; i++) {
        gpio_set_on(STEP);
        delay_us(4000);
        gpio_set_off(STEP);
        delay_us(4000);
    }
}
