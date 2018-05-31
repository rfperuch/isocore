//
// Copyright (c) 2018, Enrico Gregori, Alessandro Improta, Luca Sani, Institute
// of Informatics and Telematics of the Italian National Research Council
// (IIT-CNR). All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE IIT-CNR BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef ISOLARIO_FILTERPACKET_H_
#define ISOLARIO_FILTERPACKET_H_

#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <isolario/mrt.h>
#include <isolario/bgp.h>
#include <isolario/patriciatrie.h>

enum {
    K_MAX = 32,  // maximum user-defined filter constants

    KBUFSIZ = 64,
    STACKBUFSIZ = 256,

    BLKSTACKSIZ = 32
};

enum {
    VM_TMPTRIE,
    VM_TMPTRIE6
};

enum {
    VM_FUNCS_MAX = 16,

    VM_WITHDRAWN_INSERT_FN, VM_WITHDRAWN_ACCUMULATE_FN,
    VM_ALL_WITHDRAWN_INSERT_FN, VM_ALL_WITHDRAWN_ACCUMULATE_FN,
    VM_NLRI_INSERT_FN, VM_NLRI_ACCUMULATE_FN,
    VM_ALL_NLRI_INSERT_FN, VM_ALL_NLRI_ACCUMULATE_FN,

    VM_FUNCS_COUNT
};

typedef union {
    netaddr_t addr;
    uint32_t as;
    community_t comm;
    ex_community_t excomm;
    large_community_t lrgcomm;
    int value;
} stack_cell_t;

typedef uint16_t bytecode_t;

typedef struct filter_vm_s filter_vm_t;

typedef void (*filter_func_t)(filter_vm_t *vm);

enum {
    VM_SHORTCIRCUIT_FORCE_FLAG = 1 << 0
};

typedef struct filter_vm_s {
    bgp_msg_t *bgp;
    mrt_msg_t *mrt;
    patricia_trie_t *curtrie, *curtrie6;
    stack_cell_t *sp, *kp;  // stack and constant segment pointers
    patricia_trie_t *tries;
    filter_func_t funcs[VM_FUNCS_COUNT];
    unsigned short flags;

    // private state
    unsigned short pc;
    unsigned short si;    // stack index
    unsigned short ntries;
    unsigned short maxtries;
    unsigned short stacksiz;
    unsigned short codesiz;
    unsigned short maxcode;
    unsigned short ksiz;
    unsigned short maxk;
    unsigned short curblk;
    unsigned short blkstack[BLKSTACKSIZ];
    stack_cell_t stackbuf[STACKBUFSIZ];
    stack_cell_t kbuf[KBUFSIZ];
    patricia_trie_t triebuf[2];
    bytecode_t *code;
    jmp_buf except;
    int error;
} filter_vm_t;

typedef struct {
    int opidx;
    void *pkt_data;
    size_t datasiz;
} match_result_t;

enum {
    VM_OUT_OF_MEMORY    = -1,
    VM_STACK_OVERFLOW   = -2,
    VM_STACK_UNDERFLOW  = -3,
    VM_FUNC_UNDEFINED   = -4,
    VM_K_UNDEFINED      = -5,
    VM_TRIE_MISMATCH    = -6,
    VM_TRIE_UNDEFINED   = -7,
    VM_PACKET_MISMATCH  = -8,
    VM_BAD_PACKET       = -9,
    VM_ILLEGAL_OPCODE   = -10,
    VM_BAD_BLOCK        = -11,
    VM_BLOCKS_OVERFLOW  = -12,
    VM_SURPRISING_BYTES = -13
};

inline char *filter_strerror(int err)
{
    if (err > 0)
        return "Pass";
    if (err == 0)
        return "Fail";

    switch (err) {
    case VM_OUT_OF_MEMORY:
        return "Out of memory";
    case VM_STACK_OVERFLOW:
        return "Stack overflow";
    case VM_STACK_UNDERFLOW:
        return "Stack underflow";
    case VM_FUNC_UNDEFINED:
        return "Reference to undefined function";
    case VM_K_UNDEFINED:
        return "Reference to undefined constant";
    case VM_TRIE_MISMATCH:
        return "Trie/Prefix family mismatch";
    case VM_TRIE_UNDEFINED:
        return "Reference to undefined trie";
    case VM_PACKET_MISMATCH:
        return "Mismatched packet type for this filter";
    case VM_BAD_PACKET:
        return "Packet corruption detected";
    case VM_ILLEGAL_OPCODE:
        return "Illegal instruction";
    case VM_BAD_BLOCK:
        return "BLK instruction targets out of bounds code";
    case VM_BLOCKS_OVERFLOW:
        return "Blocks overflow, too many nested blocks";
    case VM_SURPRISING_BYTES:
        return "Sorry, I cannot make sense of these bytes";
    default:
        return "<Unknown error>";
    }
}

void filter_init(filter_vm_t *vm);

void filter_clear_error(void);

/// @brief Obtain last compilation error message.
const char *filter_last_error(void);

void filter_dump(FILE *f, filter_vm_t *vm);

int filter_compilefv(FILE *f, filter_vm_t *vm, va_list va);

int filter_compilef(FILE *f, filter_vm_t *vm, ...);

int filter_compilev(filter_vm_t *vm, const char *program, va_list va);

int filter_compile(filter_vm_t *vm, const char *program, ...);

int mrt_filter_r(mrt_msg_t *msg, filter_vm_t *vm, match_result_t *results, size_t *nresults);

int mrt_filter(filter_vm_t *vm, match_result_t *results, size_t *nresults);

int bgp_filter_r(bgp_msg_t *msg, filter_vm_t *vm, match_result_t *results, size_t *nresults);

int bgp_filter(filter_vm_t *vm, match_result_t *results, size_t *nresults);

void filter_destroy(filter_vm_t *vm);

#endif

