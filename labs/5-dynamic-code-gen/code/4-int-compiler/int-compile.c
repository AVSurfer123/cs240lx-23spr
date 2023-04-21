#include "rpi.h"

#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#include "cycle-util.h"

#include "bit-support.h"
uint32_t branch_to_imm(uint32_t branch_addr, uint32_t func_addr) {
    uint32_t imm = func_addr - branch_addr - 8;
    imm = imm >> 2;
    imm = bits_clr(imm, 24, 31);
    return imm;
}

uint32_t imm_to_branch(uint32_t branch_addr, uint32_t imm) {
    // sign extension
    if (bit_get(imm, 23)) {
        imm = bits_set(imm, 24, 29, 0b111111);
    }
    imm = imm << 2;
    return branch_addr + 8 + imm;
}

typedef void (*int_fp)(void);

static volatile unsigned cnt = 0;

// fake little "interrupt" handlers: useful just for measurement.
void int_0() { cnt++; }
void int_1() { cnt++; }
void int_2() { cnt++; }
void int_3() { cnt++; }
void int_4() { cnt++; }
void int_5() { cnt++; }
void int_6() { cnt++; }
void int_7() { cnt++; }

void generic_call_int(int_fp *intv, unsigned n) { 
    for(unsigned i = 0; i < n; i++)
        intv[i]();
}

// you will generate this dynamically.
void specialized_call_int(void) {
    int_0();
    int_1();
    int_2();
    int_3();
    int_4();
    int_5();
    int_6();
    int_7();
}

int_fp int_compile(int_fp* interrupts, unsigned n) {
    static uint32_t code[32];
    assert((n + 2) < 32);
    code[0] = 0xe92d4010; // push {r4, lr}
    printk("Found %d handlers\n", n);
    for (int i = 1; i <= n; i++) {
        uint32_t branch_addr = (uint32_t) &code[i];
        uint32_t func_addr = (uint32_t) interrupts[i - 1];
        uint32_t imm = branch_to_imm(branch_addr, func_addr);
        uint32_t computed = imm_to_branch(branch_addr, imm);
        printk("branch %x func %x Immediate %x computed %x\n", branch_addr, func_addr, imm, computed);
        code[i] = 0xeb000000 | imm; // bl int
    }
    code[n+1] = 0xe8bd8010; // pop {r4, pc}
    return (int_fp) &code;
}

int_fp jump_thread(int_fp* interrupts, unsigned n) {
    for (int i = 0; i < n - 1; i++) {
        uint32_t* start = (uint32_t*) interrupts[i];
        printk("Start %x \n", start);
        while (*start) {
            if (*start == 0xe12fff1e) { // bx lr
                uint32_t branch_addr = (uint32_t) start;
                uint32_t func_addr = (uint32_t) interrupts[i + 1];
                uint32_t imm = branch_to_imm(branch_addr, func_addr);
                uint32_t computed = imm_to_branch(branch_addr, imm);
                printk("branch %x func %x Immediate %x computed %x\n", branch_addr, func_addr, imm, computed);
                *start = 0xea000000 | imm; // b int
                break;
            }
            start++;
        }
    }
    static uint32_t code[32];
    // code[0] = 0xe92d4010; // push {r4, lr}
    uint32_t branch_addr = (uint32_t) &code[0];
    uint32_t func_addr = (uint32_t) interrupts[0];
    uint32_t imm = branch_to_imm(branch_addr, func_addr);
    uint32_t computed = imm_to_branch(branch_addr, imm);
    printk("branch %x func %x Immediate %x computed %x\n", branch_addr, func_addr, imm, computed);
    code[0] = 0xea000000 | imm; // b int
    // code[2] = 0xe8bd8010; // pop {r4, pc}
    return (int_fp) &code;
}

void notmain(void) {
    int_fp intv[] = {
        int_0,
        int_1,
        int_2,
        int_3,
        int_4,
        int_5,
        int_6,
        int_7
    };

    cycle_cnt_init();

    unsigned n = NELEM(intv);

    // try with and without cache: but if you modify the routines to do 
    // jump-threadig, must either:
    //  1. generate code when cache is off.
    //  2. invalidate cache before use.
    // enable_cache();

    cnt = 0;
    TIME_CYC_PRINT10("cost of generic-int calling",  generic_call_int(intv,n));
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

    // rewrite to generate specialized caller dynamically.
    // int_fp fp = int_compile(intv, n);
    int_fp fp = jump_thread(intv, n);

    cnt = 0;
    TIME_CYC_PRINT10("cost of specialized int calling", fp() );
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

    clean_reboot();
}
