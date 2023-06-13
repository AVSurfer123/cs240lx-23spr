#include "stepper.h"
#include "rpi.h"
#include "math-helpers.h"
#include "bit-support.h"

stepper_t * stepper_init(unsigned dir, unsigned step){
    // kmalloc_init();
    stepper_t * stepper = kmalloc(sizeof(stepper_t));
    stepper->step_count = 0;
    stepper->dir = dir;
    stepper->step = step;
    gpio_set_output(dir);
    gpio_set_output(step);
    return stepper;
}

// If you want to do microstep extension:
void stepper_set_microsteps(stepper_t * stepper, stepper_microstep_mode_t microstep_mode){
    int order;
    switch(microstep_mode) {
        case FULL_STEP:
            stepper->resolution = 1;
            order = 0b000;
            break;
        case HALF_STEP:
            stepper->resolution = 2;
            order = 0b100;
            break;
        case QUARTER_STEP:
            stepper->resolution = 4;
            order = 0b010;
            break;
        case EIGHTH_STEP:
            stepper->resolution = 8;
            order = 0b110;
            break;
        case SIXTEENTH_STEP:
            stepper->resolution = 16;
            order = 0b111;
            break;
        default:
            panic("Provided invalid stepper resolution %d\n", stepper->resolution);
    }
    gpio_write(stepper->MS1, bit_get(order, 2));
    gpio_write(stepper->MS2, bit_get(order, 1));
    gpio_write(stepper->MS3, bit_get(order, 0));
}

// If you want to do microstep extension:
stepper_t * stepper_init_with_microsteps(unsigned dir, unsigned step, unsigned MS1, unsigned MS2, unsigned MS3, stepper_microstep_mode_t microstep_mode){
    stepper_t* stepper = stepper_init(dir, step);
    stepper->MS1 = MS1;
    stepper->MS2 = MS2;
    stepper->MS3 = MS3;
    gpio_set_output(MS1);
    gpio_set_output(MS2);
    gpio_set_output(MS3);
    stepper_set_microsteps(stepper, microstep_mode);
    return stepper;
}

// how many gpio writes should you do?
void stepper_step_forward(stepper_t * stepper){
    gpio_set_on(stepper->dir);
    delay_us(1);
    gpio_set_on(stepper->step);
    delay_us(1000);
    gpio_set_off(stepper->step);
    stepper->step_count++;
}

void stepper_step_backward(stepper_t * stepper){
    gpio_set_off(stepper->dir);
    delay_us(1);
    gpio_set_on(stepper->step);
    delay_us(1000);
    gpio_set_off(stepper->step);
    stepper->step_count--;
}

int stepper_get_position_in_steps(stepper_t * stepper){
    return stepper->step_count;
}

void stepper_freq(stepper_t* stepper, double freq) {
    gpio_set_on(stepper->dir);

    int delay_time = 1000000.0 / freq / 2;
    gpio_set_on(stepper->step);
    delay_us(delay_time);
    gpio_set_off(stepper->step);
    delay_us(delay_time);
    stepper->step_count++;
}