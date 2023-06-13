#include "rpi.h"
#include "rpi-interrupts.h"
#include "rpi-armtimer.h"
#include "timer-interrupt.h"
#include "rpi-inline-asm.h"

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
    dev_barrier();
    int x = ((*(volatile uint32_t*) gpio_lev0) >> pin) & 1;
    dev_barrier();
    return x;
}

static unsigned start;
static unsigned max_cyc;
static int run;


// implement this code and tune it.
unsigned scope(unsigned pin, log_ent_t *l, unsigned n_max, unsigned max_cycles) {
    unsigned v1, v0 = gpio_read_raw(pin);

    // spin until the pin changes.
    while((v1 = gpio_read_raw(pin)) == v0)
        ;
    v0 = v1;

    // when we started sampling 
    start = cycle_cnt_read();
    max_cyc = max_cycles;
    run = 1;

    // sample until record max samples or until exceed <max_cycles>
    unsigned n = 0;
    printk("starting loop\n");
    while(run) {      

        // write this code first: record sample when the pin
        // changes.  then start tuning the whole routine.
        if((v1 = gpio_read_raw(pin)) != v0) {
            l[n] = (log_ent_t) {v0, cycle_cnt_read() - start};
            v0 = v1;
            n++;

            // if (n == n_max) {
            //     return n;
            // }
        }
    }
    printk("Returned from scope: %d\n", n);
    return n;
}

void interrupt_vector(unsigned pc) {
    dev_barrier();
    if(((*(volatile uint32_t*) IRQ_basic_pending) & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        return;
    *(volatile uint32_t*) arm_timer_IRQClear = 1;

    if (run) {
        // printk("In interrupt\n");
        if (cycle_cnt_read() - start >= max_cyc) {
            printk("run is 0\n");
            run = 0;
        }
    }
    dev_barrier();    
}

void notmain(void) {
    // setup input pin.
    int pin = 21;
    gpio_set_input(pin);

    // make sure to init cycle counter hw.
    cycle_cnt_init();
    // caches_enable();

    extern uint32_t _interrupt_table[];
    int_init();
    timer_interrupt_init(10000);
    cpsr_int_enable();

#   define MAXSAMPLES 32
    log_ent_t log[MAXSAMPLES];

    // just to illustrate.  remove this.
    // sample_ex(log, 10, CYCLE_PER_FLIP);

    // run 4 times before rebooting: makes things easier.
    // you can get rid of this.
    for(int i = 0; i < 1; i++) {
        unsigned n = scope(pin, log, MAXSAMPLES, sec_to_cycle(1));
        dump_samples(log, n, CYCLE_PER_FLIP);
    }
    clean_reboot();
}
