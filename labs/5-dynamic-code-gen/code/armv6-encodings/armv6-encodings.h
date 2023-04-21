#ifndef __ARMV6_ENCODINGS_H__
#define __ARMV6_ENCODINGS_H__

enum {
    armv6_lr = 14,
    armv6_pc = 15,
    armv6_sp = 13,
    armv6_r0 = 0,

    op_mov = 0b1101,
    armv6_mvn = 0b1111,
    armv6_orr = 0b1100,
    op_mult = 0b0000,
    code_mult = 0b1001,

    cond_always = 0b1110,

    op_bx = 0x12,
};

// trivial wrapper for register values so type-checking works better.
typedef struct {
    uint8_t reg;
} reg_t;
static inline reg_t reg_mk(unsigned r) {
    if(r >= 16)
        panic("illegal reg %d\n", r);
    return (reg_t){ .reg = r };
}

// how do you add a negative? do a sub?
static inline uint32_t armv6_mov(reg_t rd, reg_t rn) {
    // could return an error.  could get rid of checks.
    //         I        opcode        | rd
    return cond_always << 28
        | op_mov << 21 
        | rd.reg << 12 
        | rn.reg
        ;
}

static inline uint32_t 
armv6_mov_imm8_rot4(reg_t rd, uint32_t imm8, unsigned rot4) {
    if(imm8>>8)
        panic("immediate %d does not fit in 8 bits!\n", imm8);
    if(rot4 % 2)
        panic("rotation %d must be divisible by 2!\n", rot4);
    rot4 /= 2;
    if(rot4>>4)
        panic("rotation %d does not fit in 4 bits!\n", rot4);

    return cond_always << 28 | 1 << 25 | op_mov << 21 | rd.reg << 12 | rot4 << 8 | imm8;
}
static inline uint32_t 
armv6_mov_imm8(reg_t rd, uint32_t imm8) {
    return armv6_mov_imm8_rot4(rd,imm8,0);
}

static inline uint32_t 
armv6_mvn_imm8(reg_t rd, uint32_t imm8) {
    todo("implement mvn\n");
}

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

static inline uint32_t 
armv6_bx(reg_t rd) {
    return cond_always << 28 | op_bx << 20 | 1 << 4 | rd.reg;
}

// use 8-bit immediate imm8, with a 4-bit rotation.
static inline uint32_t 
armv6_orr_imm8_rot4(reg_t rd, reg_t rn, unsigned imm8, unsigned rot4) {
    if(imm8>>8)
        panic("immediate %d does not fit in 8 bits!\n", imm8);
    if(rot4 % 2)
        panic("rotation %d must be divisible by 2!\n", rot4);
    rot4 /= 2;
    if(rot4>>4)
        panic("rotation %d does not fit in 4 bits!\n", rot4);

    return cond_always << 28 | 1 << 25 | armv6_orr << 21 | rn.reg << 16 | rd.reg << 12 | rot4 << 8 | imm8;
}

static inline uint32_t 
armv6_orr_imm8(reg_t rd, reg_t rn, unsigned imm8) {
    if(imm8>>8)
        panic("immediate %d does not fit in 8 bits!\n", imm8);
    return armv6_orr_imm8_rot4(rd, rn, imm8, 0);
}

// a4-80
static inline uint32_t 
armv6_mult(reg_t rd, reg_t rm, reg_t rs) {
    return cond_always << 28 | rd.reg << 16 | rs.reg << 8 | code_mult << 4 | rm.reg;
}


// load a word from memory[offset]
// ldr rd, [rn,#offset]
static inline uint32_t 
armv6_ldr_off12(reg_t rd, reg_t rn, int offset) {
    // a5-20
    int sign = offset > 0 ? 1 : 0;
    return cond_always << 28 | 1 << 26 | 1 << 24 | sign << 23 | 1 << 20 | rn.reg << 16 | rd.reg << 12 | (offset > 0 ? offset : -offset);
}

/**********************************************************************
 * synthetic instructions.
 * 
 * these can result in multiple instructions generated so we have to 
 * pass in a location to store them into.
 */

static inline uint32_t *
armv6_load_imm32(uint32_t *code, reg_t rd, uint32_t imm32) {
    *code++ = armv6_mov_imm8(rd, bits_get(imm32, 0, 7));
    for (int i = 1; i < 4; i++) {
        uint32_t imm = bits_get(imm32, 8*i, 8*(i+1) - 1);
        if (imm != 0) {
            *code++ = armv6_orr_imm8_rot4(rd, rd, imm, 32 - 8*i);
        }
    }
    return code;
}

// MLA (Multiply Accumulate) multiplies two signed or unsigned 32-bit
// values, and adds a third 32-bit value.
//
// a4-66: multiply accumulate.
//      rd = rm * rs + rn.
static inline uint32_t
armv6_mla(reg_t rd, reg_t rm, reg_t rs, reg_t rn) {    
    return cond_always << 28 | 1 << 21 | rd.reg << 16 | rn.reg << 12 | rs.reg << 8 | code_mult << 4 | rm.reg;
}

#endif
