#include "stepper.h"

void notmain() {
    stepper_init();

    uint32_t start = timer_get_usec();
    uint32_t total = 5000000;
    uint32_t delta = total / 10;
    uint32_t last = -1;
    while (timer_get_usec() - start < total) {
        // gpio_set_on(DIR);
        // gpio_set_on(STEP);
        // delay_us(75);
        // gpio_set_off(STEP);
        // delay_us(75);

        uint32_t idx = (timer_get_usec() - start) / delta;
        double power = (idx + 1) * .1;
        if (idx != last) {
            printk("Index %d power %f\n", idx, power);
            last = idx;
        }
        // Can't add continuous print statements since it takes 2000 us which is longer than delay
        // printk("Power: %f\n", power);
        run_stepper(power);
    }

    // 200 steps = 1 rev
    // for (int i = 0; i < 200; i++) {
    //     step(4, 1);
    //     printk("Step %d\n", i);
    // }

    // while (timer_get_usec() - start < 1000000) {
    //     run_stepper(.2);
    // }
    // while (timer_get_usec() - start < 2000000) {
    //     run_stepper(.4);
    // }
}

