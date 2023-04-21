// test for load_imm32: load a 32-bit unsigned constant.
#include "rpi.h"
#include "asm-helpers.h"
#include "bit-support.h"
#include "pi-random.h"
#include "armv6-encodings.h"

enum { ntrials = 4096 };

void notmain(void) {

    uint32_t code[12];
    uint32_t (*fp)(void) = (void*)code;

    reg_t lr = reg_mk(armv6_lr);
    reg_t r0 = reg_mk(0);

    reg_t r1 = reg_mk(1);
    reg_t r2 = reg_mk(2);
    reg_t r3 = reg_mk(3);

    // generate a simple routine to return an 8bit
    // constant.  relies on no icache being enabled.
    for(unsigned i = 0; i < ntrials; i++) {
        uint32_t *cp = code;
        uint32_t imm32 = pi_random();

        cp = armv6_load_imm32(cp, r0, imm32);
        cp = armv6_load_imm32(cp, r1, imm32);
        cp = armv6_load_imm32(cp, r2, imm32);
        cp = armv6_load_imm32(cp, r3, imm32);
        *cp++ = armv6_mla(r0, r1, r2, r3);
        *cp++ = armv6_bx(lr);
        prefetch_flush();

        uint32_t got = fp();
        if(imm32*imm32 + imm32 != got)
            panic("expected=%x, got=%d\n", imm32, got);
    }
    trace("SUCCESS: trials=%d: load_imm32 seems to work\n", ntrials);
}
