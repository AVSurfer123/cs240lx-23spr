#include "rpi.h"
#include "rpi-thread.h"
#include "cycle-count.h"
#include "cycle-util.h"
#include "ir-put-get.h"

#include "fast-hash32.h"


// read a bit:
//   1. record how long IR receiver it down.
//   2. if closer to <burst_0> cycles, return 0.
//   3. if closer to <burst_1> cycles, return 1.
//
// suggested modifications:
//  - panic if read 0 for "too long" (e.g., to catch
//    a bad jumper.
//  - ignore very short 1s (which can happen if the sender
//    screws up transmission timing).
int tsop_read_bit(unsigned pin) {
    // Haven't received anything yet, or in quiet phase
    while (gpio_read(pin) == 1) {
        rpi_yield();
    }
    unsigned start = cycle_cnt_read();
    while (gpio_read(pin) == 0) {
        if (cycle_cnt_read() - start > tsop_cycle * 100000) {
            panic("Not receiving 1 from IR receiver again");
        }
        rpi_yield();
    }
    unsigned num = cycle_cnt_read() - start;
    int middle = (burst_0 + burst_1) / 2;
    return num < middle;
}


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

        write_for_ncycles_inc(rpi_yield, pin, 1, start, n += tsop_cycle);
        write_for_ncycles_inc(rpi_yield, pin, 0, start, n += tsop_cycle);

    } while((cycle_cnt_read() - start) < end);
}

int lock = 0;

void ir_put8(unsigned pin, uint8_t c) {
    while (lock) {
        rpi_yield();
    }
    lock = 1;
    delay_cycles(quiet_0 + burst_0);
    for (int i = 0; i < 8; i++) {
        if ((c >> i) & 1) {
            tsop_write_cyc(pin, burst_1 / CYC_PER_USEC);
            unsigned start = cycle_cnt_read();
            wait_until_ncycles(rpi_yield, start, quiet_1);
        }
        else {
            tsop_write_cyc(pin, burst_0 / CYC_PER_USEC);
            unsigned start = cycle_cnt_read();
            wait_until_ncycles(rpi_yield, start, quiet_0);
        }
    }
    lock = 0;
}

uint8_t ir_get8(unsigned pin) {
    uint8_t b = 0;
    for (int i = 0; i < 8; i++) {
        b |= tsop_read_bit(pin) << i;
    }
    return b;
}

void ir_put32(unsigned pin, uint32_t x) {
    for (int i = 0; i < 4; i++) {
        ir_put8(pin, (x >> (i * 8)) & 0xFF);
    }
}

uint32_t ir_get32(unsigned pin) {
    uint32_t b = 0;
    for (int i = 0; i < 4; i++) {
        b |= ir_get8(pin) << (i*8);
    }
    return b;
}

int transmit_lock = 0;

// bad form to have in/out pins globally.
int ir_send_pkt(void *data, uint32_t nbytes) {
    while(transmit_lock) {
        rpi_yield();
    }
    transmit_lock = 1;
    ir_put32(out_pin, PKT_HDR);
    uint32_t hash = fast_hash_inc32(data, nbytes, 0);
    ir_put32(out_pin, hash);
    ir_put32(out_pin, nbytes);
    transmit_lock = 0;

    uint32_t ack = ir_get32(in_pin);
    assert(ack == PKT_HDR_ACK);
    
    while(transmit_lock) {
        rpi_yield();
    }
    transmit_lock = 1;
    ir_put32(out_pin, PKT_DATA);
    uint8_t* ptr = (uint8_t*) data;
    for (int i = 0; i < nbytes; i++) {
        ir_put8(out_pin, ptr[i]);
    }
    transmit_lock = 0;
    
    ack = ir_get32(in_pin);
    assert(ack == PKT_DATA_ACK);
    return nbytes;
}

int ir_recv_pkt(void *data, uint32_t max_nbytes) {
    uint32_t hdr = ir_get32(in_pin);
    assert(hdr == PKT_HDR);
    uint32_t hash = ir_get32(in_pin);
    uint32_t nbytes = ir_get32(in_pin);

    while(transmit_lock) {
        rpi_yield();
    }
    transmit_lock = 1;
    ir_put32(out_pin, PKT_HDR_ACK);
    transmit_lock = 0;

    uint32_t msg = ir_get32(in_pin);
    assert(msg == PKT_DATA);
    uint8_t* ptr = (uint8_t*) data;
    for (int i = 0; i < nbytes && i < max_nbytes; i++) {
        ptr[i] = ir_get8(in_pin);
    }
    uint32_t computed_hash = fast_hash_inc32(data, nbytes, 0);
    assert(hash == computed_hash);

    while(transmit_lock) {
        rpi_yield();
    }
    transmit_lock = 1;
    ir_put32(out_pin, PKT_DATA_ACK);
    transmit_lock = 0;
    return nbytes;
}
