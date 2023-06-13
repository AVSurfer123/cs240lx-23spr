// simple example test generator: a lot of error.  you should reduce it.
#include "rpi.h"
#include "test-gen.h"
#include "cycle-count.h"
#include "cycle-util.h"

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
static inline void gpio_write_raw(unsigned pin, int v) {
    if (v) {
        *(volatile uint32_t*) gpio_set0 = 1 << pin;
    }
    else {
        *(volatile uint32_t*) gpio_clr0 = 1 << pin;
    }
}

// send N samples at <ncycle> cycles each in a simple way.
// a bunch of error sources here.
void test_gen(unsigned pin, unsigned N, unsigned ncycle) {
    unsigned ndelay = 0;
    unsigned start = cycle_cnt_read();

    unsigned v = 1;
    for(unsigned i = 0; i < N; i++) {
        gpio_write_raw(pin, v);
        v = 1-v;
        ndelay += ncycle;
        // we are not sure: are we delaying too much or too little?
        delay_ncycles(start, ndelay);
    }
    unsigned end = cycle_cnt_read();
    printk("expected %d cycles, have %d, v=%d\n", ncycle*N, end-start,v);
}

void notmain(void) {
    int pin = 21;
    gpio_set_output(pin);
    cycle_cnt_init();

    // keep it seperate so easy to look at assembly.
    test_gen(pin, 11, CYCLE_PER_FLIP);

    clean_reboot();
}
