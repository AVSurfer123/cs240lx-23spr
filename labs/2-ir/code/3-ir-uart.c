// BEFORE YOU DO ANYTHING: READ THIS: 
//      we are not using resistors so it's easy to fry the IR
//      LED we have.   NEVER leave it on for long!  Never plug right 
//      into 3v (or, quick smoke: 5v)
//
// simple loopback test of IR sensor and IR led.   we use two threads:
//   1. one thread transmits.
//   2. one thread receives.
// the key idea is that when a thread is busy waiting for something to 
// happen, rather than spin in loop, it calls yield to run the other thread.  
// if the cswitch overhead is low enough this will give you enough
// precision to send and receive to yourself.
//      good extension challenge: as your context switch and thread 
//      code gets faster and fasterr you can shrink your error down
//      (and the granularity you can interleave) down more and more.
//
// with a bit of work, you can rewrite this to use a run-to-completion 
// routine rather than threads.  this will be less overhead.  it will
// also be more annoying for state management.  however, a good thing
// is after doing this, converting to interrupts won't be that hard.
// ***HOWEVER: interrupts have significant overhead on the arm1176,
// so without care, they will dramatically mess with your LED transmission.
//
#include "rpi.h"
#include "rpi-thread.h"
#include "cycle-count.h"

enum {
    in_pin = 21,
    out_pin = 20 
};

// ir receiver:
//    38khz cycles --- so full transition up and down 38*1000 times per sec
// compute the number of cycles.
//
// usec = (1000*1000usec) / (38*1000) = 26.31
// cycles = (700*1000*1000) / (38*1000) = 18421.0 / 2 = 9210
//
// we go with cycles just b/c
enum { tsop_hold_cyc = 9210, };

// run until <ncycles> have passed since <start>, interleaving
// calls to <th>
static inline uint32_t
wait_until_ncycles(void (*fn)(void), uint32_t start, unsigned ncycles) {
    uint32_t n = 0;

    while(1) {
        n = cycle_cnt_read() - start;
        if(n >= ncycles)
            break;

        // yield to other thread.
        fn();
    }
    return n;
}

// set GPIO <pin> = <v> and wait <ncycles> from <start>, 
// interleaving calls to <th>
static inline void
write_for_ncycles_inc(void (*fn)(void), unsigned pin, unsigned v,
                    unsigned start, unsigned ncycles) {
    gpio_write(pin, v);     // inline this?
    wait_until_ncycles(fn, start, ncycles);
}

// Sets receiver gpio to 0
static inline void tsop_write_cyc(unsigned pin, unsigned usec) {

    unsigned end = usec_to_cycles(usec);
    unsigned n = 0;
    unsigned start = cycle_cnt_read();

    // tsop expects a 38khz signal --- this means 38,000 transitions
    // between hi (1) and lo (0) per second.
    //
    // common mistake: setting to hi or lo 38k times per sec rather 
    // than both hi+low (which is 2x as frequent).
    //
    // we use cycles rather than usec for increased precision, but 
    // at the rate the IR works, usec might be ok.
    do {

        write_for_ncycles_inc(rpi_yield, pin, 1, start, n += tsop_hold_cyc);
        write_for_ncycles_inc(rpi_yield, pin, 0, start, n += tsop_hold_cyc);

    } while((cycle_cnt_read() - start) < end);
}

#define USEC_PER_BIT 420
#define CYC_PER_BIT usec_to_cycles(USEC_PER_BIT)

void tsop_write(unsigned pin, unsigned bit) {
    if (bit) {
        unsigned start = cycle_cnt_read();
        wait_until_ncycles(rpi_yield, start, CYC_PER_BIT);
    }
    else {
        tsop_write_cyc(pin, USEC_PER_BIT);
    }
}

void ir_uart_put8(unsigned pin, uint8_t b) {
    tsop_write(pin, 0);
    for (int i = 0; i < 8; i++) {
        tsop_write(pin, (b >> i) & 1);
    }
    tsop_write(pin, 1);
}

uint8_t ir_uart_get8(unsigned pin) {
    uint32_t start = cycle_cnt_read();
    int byte = 0;
    uint32_t half = CYC_PER_BIT / 2;
    for (int i = 0; i < 8; i++) {
        wait_until_ncycles(rpi_yield, start, (i + 1) * CYC_PER_BIT + half);
        byte |= gpio_read(pin) << i;
    }
    wait_until_ncycles(rpi_yield, start, 9 * CYC_PER_BIT + half);
    // assert(gpio_read(pin) == 1); // Stop bit should be set now
    return byte;
}

// make sure we can make the IR sensor go 0 for an intended
// amount of time.
//
// AFAIK: if you don't cycle correctly, the IR will spontanously
// go back to 1.  If you see way more transitions between 1 and
// 0 this is likely what is happening.
void xmit_thread(void *input) {
    int *done = input;
    output("xmit thread\n");

    char* buf = "Hello world!\n";
    output("about to send: %s", buf);
    for(int i = 0; i < strlen(buf); i++) {
        // do nothing = 1.  do correct on/off = 0.
        ir_uart_put8(out_pin, buf[i]);
    }
    output("finished transmit loop\n");
    *done = 1;
    rpi_exit(0);
}

// while not done (*done == 0):
//  each time the IR sensor transitions from hi to lo:
//   1. measure how long 0.
//   2. print when goes back high.
//
// these should be roughly equal to the values that the transmitter
// is sending.
void recv_thread(void *input) {
    int *done = input;

    char buf[4096];
    int idx = 0;

    while(!*done) { 
        if (gpio_read(in_pin) == 1) {
            rpi_yield();
        }
        else {
            buf[idx] = ir_uart_get8(in_pin);
            idx++;
            if (buf[idx - 1] == '\n') {
                buf[idx] = 0;
                output("Got: %s", buf);
                idx = 0;
            } 
            // output("Got %c(%d) over IR uart\n", val, val);
        }
    }
    rpi_exit(0);
}

void notmain(void) {
    caches_enable();
    gpio_set_input(in_pin);
    gpio_set_pullup(in_pin);

    gpio_set_output(out_pin);
    gpio_write(out_pin, 0);

    int done = 0;
    rpi_fork(recv_thread,&done)->annot = "recv";
    rpi_fork(xmit_thread,&done)->annot = "xmit";
    rpi_thread_start();
    output("done running\n");
}
