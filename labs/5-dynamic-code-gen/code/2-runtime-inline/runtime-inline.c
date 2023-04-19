// test framework to check and time runtime inlining of GET32 and PUT32.
#include "rpi.h"
#include "cycle-count.h"
#include "bit-support.h"

// prototypes: these are identical to get32/put32.

// *(unsigned *)addr
unsigned GET32_inline(unsigned addr);
unsigned get32_inline(const volatile void *addr);

// *(unsigned *)addr = v;
void PUT32_inline(unsigned addr, unsigned v);
void put32_inline(volatile void *addr, unsigned v);

static int inline_on_p = 0, inline_cnt = 0;

// turn off inlining if we are dying.
#define die(msg...) do { inline_on_p = 0; panic("DYING:" msg); } while(0)

// inline helper: called from asembly with the <lr> of the
// original GET32_inline call instruction.
uint32_t GET32_inline_helper(uint32_t addr, uint32_t lr) {
    if(inline_on_p) {
        // die("smash the call instruction to just do: ldr r0, [r0]\n");
        uint32_t pc = lr - 4;
        uint32_t* ptr = (uint32_t*) pc;
        *ptr = 0xe5900000;

        // don't do rewriting while we output to make debugging easier.
        inline_cnt++;
        inline_on_p = 0;
        output("GET: rewriting address=%x, inline count=%d\n", pc, inline_cnt);
        inline_on_p = 1;
    }

    // we just return the first one.  should not 
    // see again.
    return *(volatile uint32_t*)addr;
}

void PUT32_inline_helper(uint32_t addr, uint32_t val, uint32_t lr) {
    if (inline_on_p) {
        uint32_t pc = lr - 4;
        uint32_t* ptr = (uint32_t*) pc;
        *ptr = 0xe5801000;
        
        inline_cnt++;
        inline_on_p = 0;
        output("PUT: rewriting address=%x, inline count=%d\n", pc, inline_cnt);
        inline_on_p = 1;
    }
    *(uint32_t*)addr = val;
}

/********************************************************************
 * simple tests that check results.
 */

// this should get rerwitten
void test_get32_inline(unsigned n) {
    for(unsigned i = 0; i < n; i++) {
        uint32_t got = GET32_inline((uint32_t)&i);
        if(got != i) {
            inline_on_p = 0;
            panic("got %d, expected %d\n", got, i);
        }
    }
}

// this should not get rewritten initially.
void test_get32(unsigned n) {
    for(unsigned i = 0; i < n; i++) {
        uint32_t got = GET32((uint32_t)&i);
        if(got != i)
            panic("got %d, expected %d\n", got, i);
    }
}


// test using our runtime inline version.
void test_put32_inline(unsigned n) {
    uint32_t x;

    for(unsigned i = 0; i < n; i++) {
        PUT32_inline((uint32_t)&x, i);
        if(x != i)
            panic("got %d, expected %d\n", x, i);
    }
}

// test using regular put32: this won't get rewritten initially.
void test_put32(unsigned n) {
    uint32_t x;

    for(unsigned i = 0; i < n; i++) {
        PUT32((uint32_t)&x, i);
        if(x != i)
            panic("got %d, expected %d\n", x, i);
    }
}

/*************************************************************
 * versions without loops or checking: more sensitive to speedup
 */

// use our inline GET32
void test_get32_inline_10(void) {
    GET32_inline(0);
    GET32_inline(0);
    GET32_inline(0);
    GET32_inline(0);
    GET32_inline(0);

    GET32_inline(0);
    GET32_inline(0);
    GET32_inline(0);
    GET32_inline(0);
    GET32_inline(0);
}

// use the raw GET32
void test_get32_10(void) {
    GET32(0);
    GET32(0);
    GET32(0);
    GET32(0);
    GET32(0);

    GET32(0);
    GET32(0);
    GET32(0);
    GET32(0);
    GET32(0);
}




// use our inline GET32
void test_put32_inline_10(void) {
    uint32_t val;
    uint32_t addr = (uint32_t) &val;
    PUT32_inline(addr, 0);
    PUT32_inline(addr, 0);
    PUT32_inline(addr, 0);
    PUT32_inline(addr, 0);
    PUT32_inline(addr, 0);

    PUT32_inline(addr, 0);
    PUT32_inline(addr, 0);
    PUT32_inline(addr, 0);
    PUT32_inline(addr, 0);
    PUT32_inline(addr, 0);
}

// use the raw GET32
void test_put32_10(void) {
    uint32_t val;
    uint32_t addr = (uint32_t) &val;
    PUT32(addr, 0);
    PUT32(addr, 0);
    PUT32(addr, 0);
    PUT32(addr, 0);
    PUT32(addr, 0);

    PUT32(addr, 0);
    PUT32(addr, 0);
    PUT32(addr, 0);
    PUT32(addr, 0);
    PUT32(addr, 0);
}

uint32_t branch_to_imm(uint32_t branch_addr, uint32_t func_addr) {
    int imm = func_addr - branch_addr - 8;
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

void notmain2() {
    output("\n*****************************\n\n");
    output("about to test put32 inlining\n");
    inline_on_p = 1;
    uint32_t t_inline = TIME_CYC(test_put32_inline(1));
    uint32_t t_inline_run10 = TIME_CYC(test_put32_inline(10));
    uint32_t t_run10 = TIME_CYC(test_put32(10));
    inline_on_p = 0;

    output("time to run w/ inlining overhead:   %d\n", t_inline);
    output("time to run 10 times inlined:       %d\n", t_inline_run10);
    output("time to run 10 times non-inlined:   %d\n", t_run10);
    output("total inline count=%d\n", inline_cnt);


    output("about to test put32 inlining without loop or checking\n");

    // test without a loop
    inline_on_p = 1;
    t_inline       = TIME_CYC(test_put32_inline_10());
    t_inline_run10 = TIME_CYC(test_put32_inline_10());
    t_run10        = TIME_CYC(test_put32_10());
    inline_on_p = 0;

    output("time to run w/ inlining overhead:   %d\n", t_inline);
    output("time to run 10 times inlined:       %d\n", t_inline_run10);
    output("time to run 10 times non-inlined:   %d\n", t_run10);
    output("total inline count=%d\n", inline_cnt);

    // todo("smash the original put32: should get same speedup\n");
    
    // smash the GET32 code to call the GET32_inline_helper, identically
    // how GET32_inline does.  you can' just copy the code since the branch
    // is relative to the destination.
    // 
    //      note: 0xe1a0100e  =  mov r1, lr
    uint32_t *put_pc = (void *)PUT32;
    uint32_t orig_val0 = put_pc[0];
    uint32_t orig_val1 = put_pc[1];

    // todo("assign the new instructions to get_pc[0] and get_pc[1]\n");
    uint32_t branch_addr = (uint32_t) &put_pc[1];
    uint32_t get_addr = (uint32_t) PUT32_inline_helper;
    uint32_t imm = branch_to_imm(branch_addr, get_addr);
    printk("branch %x func %x Immediate %x computed %x\n", branch_addr, get_addr, imm, imm_to_branch(branch_addr, imm));

    put_pc[0] = 0xe1a0200e;
    put_pc[1] = 0xea000000 | imm;

    output("after rewriting put32!\n");
    output("    inst[0] = %x\n", put_pc[0]);
    output("    inst[1] = %x\n", put_pc[1]);
    inline_on_p = 1;

    // this test should now have inline overhead.
    t_inline       = TIME_CYC(test_put32_10());
    // this is our original: should have the same speedup.
    t_inline_run10 = TIME_CYC(test_put32_inline_10());
    // after inlining this should be same as the GET32_inline overhead.
    t_run10        = TIME_CYC(test_put32_10());
    inline_on_p = 0;

    output("time to run w/ inlining overhead:   %d\n", t_inline);
    output("time to run 10 times inlined:       %d\n", t_inline_run10);
    output("time to run 10 times non-inlined:   %d\n", t_run10);
    output("total inline count=%d\n", inline_cnt);
}

void notmain(void) {
    assert(!inline_cnt);

    output("about to test get32 inlining\n");
    inline_on_p = 1;
    uint32_t t_inline = TIME_CYC(test_get32_inline(1));
    uint32_t t_inline_run10 = TIME_CYC(test_get32_inline(10));
    uint32_t t_run10 = TIME_CYC(test_get32(10));
    inline_on_p = 0;

    output("time to run w/ inlining overhead:   %d\n", t_inline);
    output("time to run 10 times inlined:       %d\n", t_inline_run10);
    output("time to run 10 times non-inlined:   %d\n", t_run10);
    output("total inline count=%d\n", inline_cnt);


    output("about to test get32 inlining without loop or checking\n");

    // test without a loop
    inline_on_p = 1;
    t_inline       = TIME_CYC(test_get32_inline_10());
    t_inline_run10 = TIME_CYC(test_get32_inline_10());
    t_run10        = TIME_CYC(test_get32_10());
    inline_on_p = 0;

    output("time to run w/ inlining overhead:   %d\n", t_inline);
    output("time to run 10 times inlined:       %d\n", t_inline_run10);
    output("time to run 10 times non-inlined:   %d\n", t_run10);
    output("total inline count=%d\n", inline_cnt);

    // todo("smash the original get32: should get same speedup\n");
    
    // smash the GET32 code to call the GET32_inline_helper, identically
    // how GET32_inline does.  you can' just copy the code since the branch
    // is relative to the destination.
    // 
    //      note: 0xe1a0100e  =  mov r1, lr
    uint32_t *get_pc = (void *)GET32;
    uint32_t orig_val0 = get_pc[0];
    uint32_t orig_val1 = get_pc[1];

    // todo("assign the new instructions to get_pc[0] and get_pc[1]\n");
    uint32_t branch_addr = (uint32_t) &get_pc[1];
    uint32_t get_addr = (uint32_t) GET32_inline_helper;
    uint32_t imm = branch_to_imm(branch_addr, get_addr);
    printk("branch %x func %x Immediate %x computed %x\n", branch_addr, get_addr, imm, imm_to_branch(branch_addr, imm));

    get_pc[0] = 0xe1a0100e;
    get_pc[1] = 0xea000000 | imm;

    output("after rewriting get32!\n");
    output("    inst[0] = %x\n", get_pc[0]);
    output("    inst[1] = %x\n", get_pc[1]);
    inline_on_p = 1;

    // this test should now have inline overhead.
    t_inline       = TIME_CYC(test_get32_10());
    // this is our original: should have the same speedup.
    t_inline_run10 = TIME_CYC(test_get32_inline_10());
    // after inlining this should be same as the GET32_inline overhead.
    t_run10        = TIME_CYC(test_get32_10());
    inline_on_p = 0;

    output("time to run w/ inlining overhead:   %d\n", t_inline);
    output("time to run 10 times inlined:       %d\n", t_inline_run10);
    output("time to run 10 times non-inlined:   %d\n", t_run10);
    output("total inline count=%d\n", inline_cnt);

    notmain2();
}

