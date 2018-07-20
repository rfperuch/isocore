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

#include <ctype.h>
#include <isolario/filterintrin.h>
#include <isolario/filterpacket.h>
#include <isolario/parse.h>
#include <stdbool.h>
#include <stdlib.h>
#include <strings.h>

static _Thread_local char err_msg[64] = "";
static _Thread_local jmp_buf err_jmp;

enum { LEFT_TERM, RIGHT_TERM };

//
// EXPR := NOT EXPR | TERM OP TERM | CALL FN | ( EXPR ) | EXPR AND EXPR | EXPR OR EXPR
// TERM := $REG | 127.0.0.1 | 2001:db8::ff00:42:8329 | [ TERM, ... ] | ACCESSOR
//

/// @brief Handles "$constant" and "$[constant]" tokens
static int parse_registry(const char *tok, va_list va)
{
    const char *ptr = tok + 1;
    if (*ptr == '[')
        ptr++;

    if (!isdigit((unsigned char) *ptr))
        parsingerr("%s: illegal non-numeric register constant", tok);

    int idx = atoi(ptr);
    do ptr++; while (isdigit((unsigned char) *ptr));

    if (tok[1] == '[') {
        if (*ptr != ']')
            parsingerr("%s: illegal register constant, mismatched brackets", tok);

        va_list vc;
        va_copy(vc, va);
        for (int i = 0; i < idx; i++)
            va_arg(vc, int);

        idx = va_arg(vc, int);
        va_end(vc);
    }

    if (unlikely(idx > K_MAX))
        parsingerr("%s: constant register index %d is out of range", tok, idx);

    return idx;
}

static int parse_constant(filter_vm_t *vm, const char *tok, va_list va)
{
    int idx;

    if (tok[0] == '$') {
        // constant registry
        idx = parse_registry(tok, va);
    } else {
        // IPv4 or IPv6 address
        idx = vm_newk(vm);
        if (stonaddr(&vm->kp[idx].addr, tok) != 0)
            parsingerr("invalid constant value '%s'", tok);
    }

    return idx;
}

static uint64_t parse_element(filter_vm_t *vm, const char *tok, int kind, va_list va)
{
    uint64_t usage_mask = 0;

    int idx = parse_constant(vm, tok, va);
    if (kind == RIGHT_TERM) {
        if (idx <= K_MAX) {
            // this expression did not generate a new constant, we must
            // dynamically add it to the current Patricia and clean it
            // after the operation, mark it as used in the usage mask
            vm_emit(vm, vm_makeop(FOPC_LOADK, idx));
            vm_emit(vm, FOPC_STORE);

            usage_mask |= (1 << idx);
        } else {
            // this is just a regular constant expression in source code,
            // precompile it into the Patricia and avoid LOADK/STORE
            // XXX improve
            assert(idx == (int) vm->ksiz - 1);

            vm_exec_settrie(vm, vm->ntries - 2);
            vm_exec_settrie6(vm, vm->ntries - 1);
            vm_exec_loadk(vm, idx);
            vm_exec_store(vm);

            vm->ksiz--;  // constant may now be reused
        }
    }

    return usage_mask;
}

static uint64_t parse_argument(FILE *f, filter_vm_t *vm, int kind, va_list va)
{
    uint64_t usage_mask = 0;
    int is_array        = false;

    char *tok = expecttoken(f, NULL);
    if (strcmp(tok, "[") == 0)
        is_array = true;
    else
        ungettoken(tok);

    if (kind == RIGHT_TERM) {
        // preload values in tries
        int v4 = vm_newtrie(vm, AF_INET);
        int v6 = vm_newtrie(vm, AF_INET6);

        vm_emit_ex(vm, FOPC_SETTRIE, v4);
        vm_emit_ex(vm, FOPC_SETTRIE6, v6);
    }

    do {
        tok = expecttoken(f, NULL);
        if (is_array && strcmp(tok, "]") == 0)
            is_array = false;
        else
            usage_mask |= parse_element(vm, tok, kind, va);

    } while (is_array);

    return usage_mask;
}

/// @brief Compile a term, returns the trie user-custom register usage mask,
//         used registers should be DISCARDed from trie after op
static uint64_t compile_term(FILE *f, filter_vm_t *vm, int kind, va_list va)
{
    uint64_t usage_mask = 0;

    char *tok = expecttoken(f, NULL);
    if (strncasecmp(tok, "packet.", 7) == 0) {
        // packet accessor
        char *field = tok + 7;
        if (strcasecmp(field, "withdrawn") == 0) {
            bytecode_t opcode;
            if (kind == LEFT_TERM)
                opcode = vm_makeop(FOPC_CALL, VM_WITHDRAWN_ACCUMULATE_FN);
            else
                opcode = vm_makeop(FOPC_CALL, VM_WITHDRAWN_INSERT_FN);

            vm_emit(vm, opcode);
        } else if (strcasecmp(field, "every_withdrawn") == 0) {
            bytecode_t opcode;
            if (kind == LEFT_TERM)
                opcode = vm_makeop(FOPC_CALL, VM_ALL_WITHDRAWN_ACCUMULATE_FN);
            else
                opcode = vm_makeop(FOPC_CALL, VM_ALL_WITHDRAWN_INSERT_FN);

            vm_emit(vm, opcode);
        } else if (strcasecmp(field, "nlri") == 0) {
            bytecode_t opcode;
            if (kind == LEFT_TERM)
                opcode = vm_makeop(FOPC_CALL, VM_NLRI_ACCUMULATE_FN);
            else
                opcode = vm_makeop(FOPC_CALL, VM_NLRI_INSERT_FN);

            vm_emit(vm, opcode);
        } else if (strcasecmp(field, "every_nlri") == 0) {
            bytecode_t opcode;
            if (kind == LEFT_TERM)
                opcode = vm_makeop(FOPC_CALL, VM_ALL_NLRI_ACCUMULATE_FN);
            else
                opcode = vm_makeop(FOPC_CALL, VM_ALL_NLRI_INSERT_FN);

            vm_emit(vm, opcode);
        } else {
            parsingerr("unknown packet accessor '%s'", field);
        }
    } else {
        ungettoken(tok);
        usage_mask = parse_argument(f, vm, kind, va);
    }

    // XXX: would be nice to reuse code both for single constants and array...
    return usage_mask;
}

static void vm_clear_temporaries(filter_vm_t *vm, uint64_t usage_mask)
{
    for (int i = 0; i <= K_MAX; i++) {
        if (usage_mask & (1ull << i)) {
            vm_emit(vm, vm_makeop(FOPC_LOADK, i));
            vm_emit(vm, vm_makeop(FOPC_DISCARD, i));
        }
    }
}

static void compile_expr(FILE *f, filter_vm_t *vm, va_list va)
{
    while (true) {
        char *tok = expecttoken(f, NULL);

        int block_start = -1;
        if (strcasecmp(tok, "NOT") == 0) {
            compile_expr(f, vm, va);

            vm_emit(vm, FOPC_NOT);
        } else if (strcasecmp(tok, "(") == 0) {
            // nest a new block
            vm_emit(vm, FOPC_BLK);  // placeholder

            block_start = vm->codesiz;
            compile_expr(f, vm, va);
            expecttoken(f, ")");
        } else if (strcasecmp(tok, "CALL") == 0) {
            tok = expecttoken(f, NULL);

            int idx = parse_registry(tok, va);
            vm_emit_ex(vm, FOPC_CALL, idx);
        } else {
            // TERM OP TERM
            ungettoken(tok);
            compile_term(f, vm, LEFT_TERM, va);

            tok = expecttoken(f, NULL);
            bytecode_t op;
            if (strcasecmp(tok, "EXACT") == 0) {
                op = FOPC_EXACT;
            } else {
                parsingerr("unknown operation: '%s'", tok);
                break; // unreachable
            }

            uint64_t usage_mask = compile_term(f, vm, RIGHT_TERM, va);

            vm_emit(vm, op);
            // DISCARD any temporary constant loaded into Patricia
            vm_clear_temporaries(vm, usage_mask);
        }

        tok = parse(f);
        if (!tok)
            break;

        if (strcasecmp(tok, "AND") == 0) {
           vm_emit(vm, FOPC_CFAIL);
        } else if (strcasecmp(tok, "OR") == 0) {
            vm_emit(vm, FOPC_CPASS);
        } else {
            // FIXME
            ungettoken(tok);
            break;
        }

        // if we encountered a "(", we must set the BLK to jump after
        // this opcode, we must rewrite the BLK to reflect the actual
        // block size for that
        if (block_start >= 0) {
            // finalize the BLK instruction
            int blksiz = vm->codesiz - block_start;
            assert(blksiz <= 0xff); // FIXME should insert these instructions and support EXARG!
            vm->code[block_start - 1] = vm_makeop(FOPC_BLK, blksiz);
        }
    }
}

static void handle_parse_error(const char *name, unsigned int lineno, const char *msg, void *data)
{
    (void) name, (void) lineno, (void) data;

    strncpy(err_msg, msg, sizeof(err_msg) - 1);
    err_msg[sizeof(err_msg) - 1] = '\0';
    longjmp(err_jmp, -1);
}

void filter_clear_error(void)
{
    err_msg[0] = '\0';
}

const char *filter_last_error(void)
{
    return err_msg;
}

int filter_compilefv(FILE *f, filter_vm_t *vm, va_list va)
{
    filter_init(vm);
    if (setjmp(err_jmp) != 0) {
        setperrcallback(NULL);
        return -1;
    }

    setperrcallback(handle_parse_error);

    compile_expr(f, vm, va);

    setperrcallback(NULL);
    return 0;
}

int filter_compilef(FILE *f, filter_vm_t *vm, ...)
{
    va_list va;

    va_start(va, vm);
    int res = filter_compilefv(f, vm, va);
    va_end(va);
    return res;
}

int filter_compilev(filter_vm_t *vm, const char *program, va_list va)
{
    FILE *f = fmemopen((char *) program, strlen(program), "r");
    if (unlikely(!f))
        return -1;

    int res = filter_compilefv(f, vm, va);
    fclose(f);
    return res;
}

int filter_compile(filter_vm_t *vm, const char *program, ...)
{
    va_list va;
    va_start(va, program);
    int res = filter_compilev(vm, program, va);
    va_end(va);
    return res;
}
