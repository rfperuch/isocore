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
    BAD_OPCODE = -1,

    FOPC_NOP,    ///< NOP: does nothing
    FOPC_LOAD,   ///< PUSH: direct value load
    FOPC_LOADK,  ///< PUSH: load from constants environment
    FOPC_EXARG,  ///< NOP: extends previous operation's argument range
    FOPC_STORE,  ///< POP: store address into current trie (v4 or v6 depends on address)
    FOPC_NOT,    ///< POP-PUSH: pops stack topmost value and negates it
    FOPC_CPASS,  ///< POP: pops topmost stack element and terminates with PASS if value is true.
    FOPC_CFAIL,  ///< POP: pops topmost stack element and terminates with FAIL if value is false.
    FOPC_IN,     ///< POPA-PUSH: pops the entire stack and performs a IN operation, pushes the result.
    FOPC_ALL,
    FOPC_SUBNET,
    FOPC_CALL, // call function
    FOPC_SETTRIE,
    FOPC_SETTRIE6,
    FOPC_CLRTRIE,
    FOPC_CLRTRIE6
};

enum {
    K_MAX = 32,  // maximum user-defined filter constants

    KBUFSIZ = 64,
    STACKBUFSIZ = 256
};

enum {
    FILTER_FUNCS_MAX = 16,

    ACC_WITHDRAWN_LEFT, ACC_WITHDRAWN_RIGHT,
    ACC_NLRI_LEFT, ACC_NLRI_RIGHT,

    FILTER_FUNCS_COUNT
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

inline bytecode_t pack_filter_op(int opcode, int arg)
{
    return ((arg << 8) & 0xff00) | (opcode & 0xff);
}

typedef struct filter_vm_s {
    bgp_msg_t *bgp;
    mrt_msg_t *mrt;
    patricia_trie_t *curtrie, *curtrie6;
    stack_cell_t *sp, *kp;  // stack and constant segment pointers
    patricia_trie_t *tries;
    void (*funcs[FILTER_FUNCS_COUNT])(struct filter_vm_s *vm);

    // private state
    unsigned short si;    // stack index
    unsigned short ntries;
    unsigned short maxtries;
    unsigned short stacksiz;
    unsigned short codesiz;
    unsigned short maxcode;
    unsigned short ksiz;
    unsigned short maxk;
    stack_cell_t stackbuf[STACKBUFSIZ];
    stack_cell_t kbuf[KBUFSIZ];
    patricia_trie_t triebuf[2];
    bytecode_t *code;
    jmp_buf except;
} filter_vm_t;

typedef struct {
    int opidx;
    void *pkt_data;
    size_t datasiz;
} match_result_t;

void filter_init(filter_vm_t *vm);

void filter_clear_error(void);

/// @brief Obtain last compilation error message.
const char *filter_last_error(void);

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

