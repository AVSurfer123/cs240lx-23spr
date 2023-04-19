#include "rpi.h"
#include "bit-support.h"

void hello(void) { 
    printk("hello world\n");
}

// i would call this instead of printk if you have problems getting
// ldr figured out.
void foo(int x) { 
    printk("foo was passed %d\n", x);
}

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

void notmain(void) {
    // generate a dynamic call to hello world.
    // 1. you'll have to save/restore registers
    // 2. load the string address [likely using ldr]
    // 3. call printk
    static uint32_t code[16];
    static char* str = "hello world\n";
    code[0] = 0xe92d4010; // push {r4, lr}
    code[1] = 0xe59f0004; // ldr r0, [pc #4]
    uint32_t branch_addr = (uint32_t) &code[2];
    uint32_t print_addr = (uint32_t) &printk;
    uint32_t imm = branch_to_imm(branch_addr, print_addr);
    uint32_t computed = imm_to_branch(branch_addr, imm);
    printk("branch %x printk %x Immediate %x computed %x\n", branch_addr, print_addr, imm, computed);
    code[2] = 0xeb000000 | imm; // bl printk
    code[3] = 0xe8bd8010; // pop {r4, pc}
    code[4] = (uint32_t) str; // 
    
    unsigned n = 5;
    printk("emitted code:\n");
    for(int i = 0; i < n; i++) 
        printk("code[%d]=0x%x\n", i, code[i]);

    void (*fp)(void) = (typeof(fp))code;
    printk("about to call: %x\n", fp);
    printk("--------------------------------------\n");
    fp();
    printk("--------------------------------------\n");
    printk("success!\n");
    clean_reboot();
}
