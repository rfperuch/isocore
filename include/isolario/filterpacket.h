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

    KBASESIZ,  // base size for known variables

    KBUFSIZ = 64,
    STACKBUFSIZ = 256,

    BLKSTACKSIZ = 32
};

enum { VM_TMPTRIE, VM_TMPTRIE6 };

enum {
    VM_FUNCS_MAX = 16,

    VM_WITHDRAWN_INSERT_FN, VM_WITHDRAWN_ACCUMULATE_FN,
    VM_ALL_WITHDRAWN_INSERT_FN, VM_ALL_WITHDRAWN_ACCUMULATE_FN,
    VM_NLRI_INSERT_FN, VM_NLRI_ACCUMULATE_FN,
    VM_ALL_NLRI_INSERT_FN, VM_ALL_NLRI_ACCUMULATE_FN,

    VM_FUNCS_COUNT
};

typedef long long wide_as_t; ///< A type able to hold any AS32 value, plus AS_ANY

enum {
    AS_ANY = -1 ///< Special value for filter operations that matches any AS32
};

typedef union {
    netaddr_t addr;
    wide_as_t as;
    community_t comm;
    ex_community_t excomm;
    large_community_t lrgcomm;
    int value;
    struct {
        unsigned int base;
        unsigned int nels;
        unsigned int elsiz;
    };
} stack_cell_t;

typedef uint16_t bytecode_t;

typedef struct filter_vm_s filter_vm_t;

typedef void (*filter_func_t)(filter_vm_t *vm);

enum {
    VM_SHORTCIRCUIT_FORCE_FLAG = 1 << 2
};

typedef struct filter_vm_s {
    bgp_msg_t *bgp;
    patricia_trie_t *curtrie, *curtrie6;
    stack_cell_t *sp, *kp;  // stack and constant segment pointers
    patricia_trie_t *tries;
    filter_func_t funcs[VM_FUNCS_COUNT];
    unsigned short flags;         // general VM flags

    // private state
    unsigned short pc;
    unsigned short si;           // stack index
    unsigned short access_mask;  // current packet access mask
    unsigned short ntries;
    unsigned short maxtries;
    unsigned short stacksiz;
    unsigned short codesiz;
    unsigned short maxcode;
    unsigned short ksiz;
    unsigned short maxk;
    unsigned short curblk;
    stack_cell_t stackbuf[STACKBUFSIZ];
    stack_cell_t kbuf[KBUFSIZ];
    patricia_trie_t triebuf[2];
    bytecode_t *code;
    void *heap;
    unsigned int highwater;
    unsigned int dynmarker;
    unsigned int heapsiz;
    int (*settle_func)(bgp_msg_t *); // SETTLE function (if not NULL has to be called to terminate the iteration)
    int error;
    jmp_buf except;
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
    VM_BAD_ACCESSOR     = -6,
    VM_TRIE_MISMATCH    = -7,
    VM_TRIE_UNDEFINED   = -8,
    VM_PACKET_MISMATCH  = -9,
    VM_BAD_PACKET       = -10,
    VM_ILLEGAL_OPCODE   = -11,
    VM_DANGLING_BLK     = -12,
    VM_SPURIOUS_ENDBLK  = -13,
    VM_SURPRISING_BYTES = -14,
    VM_BAD_ARRAY        = -15
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
    case VM_BAD_ACCESSOR:
        return "Illegal packet accessor";
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
    case VM_DANGLING_BLK:
        return "Dangling BLK at execution end";
    case VM_SPURIOUS_ENDBLK:
        return "ENDBLK with no BLK";
    case VM_SURPRISING_BYTES:
        return "Sorry, I cannot make sense of these bytes";
    case VM_BAD_ARRAY:
        return "Array access out of bounds";
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

int bgp_filter_r(bgp_msg_t *msg, filter_vm_t *vm);

int bgp_filter(filter_vm_t *vm);

void filter_destroy(filter_vm_t *vm);

#endif
