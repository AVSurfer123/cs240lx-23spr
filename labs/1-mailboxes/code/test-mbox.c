#include "rpi.h"
#include "mbox.h"

#include "cycle-count.h"

// compute cycles per second using
//  - cycle_cnt_read();
//  - timer_get_usec();
unsigned cyc_per_sec(void) {
    uint32_t start_cyc = cycle_cnt_read();
    uint32_t start_time = timer_get_usec();
    while (timer_get_usec() - start_time < 1000000) {
    }
    return (unsigned) (cycle_cnt_read() - start_cyc);
}


void notmain(void) { 
    output("mailbox serial number = %llx\n", rpi_get_serialnum());
    // todo("implement the rest");

    output("mailbox revision number = %x\n", rpi_get_revision());
    output("mailbox model number = %x\n", rpi_get_model());

    uint32_t size = rpi_get_memsize();
    output("mailbox physical mem: size=%d (%dMB)\n", 
            size, 
            size/(1024*1024));

    // print as fahrenheit
    unsigned x = rpi_temp_get();

    // convert <x> to C and F
    unsigned C = x / 1000, F = x * 9 / 5 / 1000 + 32;
    output("mailbox temp = %x, C=%d F=%d\n", x, C, F); 

    output("Cyc per sec: %d\n", cyc_per_sec());

    const uint32_t arm_clock = 0x000000003;
    output("Min max clock rates: %d %d\n", rpi_clock_minhz_get(arm_clock), rpi_clock_maxhz_get(arm_clock));
    output("Cur clock rate: %d\n", rpi_clock_curhz_get(arm_clock));

    // printk("Setting turbo\n");
    // rpi_set_turbo();

    printk("Overclocking higher now\n");
    rpi_clock_hz_set(arm_clock, 950000000);
    output("New cyc per sec: %d\n", cyc_per_sec());
    output("Cur clock rate: %d\n", rpi_clock_curhz_get(arm_clock));

    x = rpi_temp_get();
    C = x / 1000; 
    F = x * 9 / 5 / 1000 + 32;
    output("mailbox temp = %d, C=%d F=%d\n", x, C, F); 
}
