#include "rpi.h"

// cycle counter routines.
#include "cycle-count.h"


// this defines the period: makes it easy to keep track and share
// with the test generator.
#include "../test-gen/test-gen.h"

// trivial logging code to compute the error given a known
// period.
#include "samples.h"

// derive this experimentally: check that your pi is the same!!
#define CYCLE_PER_SEC (700*1000*1000)

// some utility routines: you don't need to use them.
#include "cycle.h"

enum {
    GPIO_BASE = 0x20200000,
    gpio_set0  = (GPIO_BASE + 0x1C),
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34),
};
static inline void gpio_set_on_raw(unsigned pin) {
    *(volatile uint32_t*) gpio_set0 = 1 << pin;
}
static inline void gpio_set_off_raw(unsigned pin) {
    *(volatile uint32_t*) gpio_clr0 = 1 << pin;
}
static inline int gpio_read_raw(unsigned pin) {
    return ((*(volatile uint32_t*) gpio_lev0) >> pin) & 1;
}

// implement this code and tune it.
unsigned 
scope(unsigned pin, log_ent_t *l, unsigned n_max, unsigned max_cycles) {
    unsigned v1, v0 = gpio_read_raw(pin);

    // spin until the pin changes.
    while((v1 = gpio_read_raw(pin)) == v0)
        ;
    v0 = v1;

    // when we started sampling 
    unsigned start = cycle_cnt_read(), t = start;

    // sample until record max samples or until exceed <max_cycles>
    unsigned n = 0;
    while(1) {      
        t = cycle_cnt_read();

        // write this code first: record sample when the pin
        // changes.  then start tuning the whole routine.
        if((v1 = gpio_read_raw(pin)) != v0) {
            l[n] = (log_ent_t) {v0, t - start};
            v0 = v1;
            n++;
        }

        // exit when we have run too long.
        if((t - start) > max_cycles)  {
            printk("timeout! start=%d, t=%d, minux=%d\n",
                                 start,t,t-start);
            return n;
        }
    }
    return n;
}

void notmain(void) {
    // setup input pin.
    int pin = 21;
    gpio_set_input(pin);

    // make sure to init cycle counter hw.
    cycle_cnt_init();
    caches_enable();

#   define MAXSAMPLES 32
    log_ent_t log[MAXSAMPLES];

    // just to illustrate.  remove this.
    // sample_ex(log, 10, CYCLE_PER_FLIP);

    // run 4 times before rebooting: makes things easier.
    // you can get rid of this.
    for(int i = 0; i < 4; i++) {
        unsigned n = scope(pin, log, MAXSAMPLES, sec_to_cycle(1));
        dump_samples(log, n, CYCLE_PER_FLIP);
    }
    clean_reboot();
}
