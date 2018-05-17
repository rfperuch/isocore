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
#include <isolario/bgp.h>
#include <isolario/filterintrin.h>
#include <isolario/filterpacket.h>
#include <isolario/mrt.h>
#include <isolario/parse.h>
#include <isolario/patriciatrie.h>
#include <isolario/util.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <strings.h>

enum {
    K_GROW_STEP        = 32,
    STACK_GROW_STEP    = 128,
    CODE_GROW_STEP     = 128,
    PATRICIA_GROW_STEP = 2
};

static _Thread_local char err_msg[64] = "";
static _Thread_local jmp_buf err_jmp;

extern bytecode_t pack_filter_op(int opcode, int arg);

void filter_destroy(filter_vm_t *vm)
{
    for (unsigned int i = 0; i < vm->ntries; i++)
        patdestroy(&vm->triebuf[i]);

    if (vm->tries != vm->triebuf)
        free(vm->tries);
    if (vm->kp != vm->kbuf)
        free(vm->kp);
    if (vm->sp != vm->stackbuf)
        free(vm->sp);

    free(vm->code);
}

// static filter_regidx_t compile_expr(FILE *f, filter_vm_t *vm);

enum { LEFT_TERM, RIGHT_TERM };

//
// EXPR := NOT EXPR | TERM OP TERM | CALL FN | ( EXPR ) | EXPR AND EXPR | EXPR OR EXPR
// TERM := $REG | 127.0.0.1 | 2001:db8::ff00:42:8329 | [ TERM, ... ] | ACCESSOR
//

void vm_growstack(filter_vm_t *vm)
{
    stack_cell_t *stk = NULL;
    if (vm->sp != vm->stackbuf)
        stk = vm->sp;

    unsigned short stacksiz = vm->stacksiz + STACK_GROW_STEP;
    stk = realloc(stk, stacksiz * sizeof(*stk));
    if (unlikely(!stk))
        vm_abort(vm);

    vm->sp       = stk;
    vm->stacksiz = stacksiz;
}

static void vm_growcode(filter_vm_t *vm)
{
    unsigned short codesiz = vm->maxcode + CODE_GROW_STEP;
    bytecode_t *code = realloc(vm->code, codesiz * sizeof(*code));
    if (unlikely(!code))
        parsingerr("out of memory");

    vm->code    = code;
    vm->maxcode = codesiz;
}

static void vm_growk(filter_vm_t *vm)
{
    unsigned short ksiz = vm->maxk + K_GROW_STEP;
    stack_cell_t *k = NULL;
    if (k != vm->kbuf)
        k = vm->kp;

    k = realloc(k, ksiz * sizeof(*k));
    if (unlikely(!k))
        parsingerr("out of memory");

    vm->kp   = k;
    vm->maxk = ksiz;
}

static void vm_growtrie(filter_vm_t *vm)
{
    patricia_trie_t *tries = NULL;
    if (vm->tries != vm->triebuf)
        tries = vm->tries;

    unsigned short ntries = vm->maxtries + PATRICIA_GROW_STEP;
    tries = realloc(tries, ntries * sizeof(*tries));
    if (unlikely(!tries))
        parsingerr("out of memory");

    vm->tries    = tries;
    vm->maxtries = ntries;
}

extern void vm_clearstack(filter_vm_t *vm);

extern noreturn void vm_abort(filter_vm_t *vm);

/// @brief Emit one bytecode operation
static void vm_emit(filter_vm_t *vm, bytecode_t opcode)
{
    if (unlikely(vm->codesiz == vm->maxcode))
        vm_growcode(vm);

    vm->code[vm->codesiz++] = opcode;
}

/// @brief Reserve one constant (avoids the user custom section).
static int vm_newk(filter_vm_t *vm)
{
    if (unlikely(vm->ksiz == vm->maxk))
        vm_growk(vm);

    return vm->ksiz++;
}

static int vm_newtrie(filter_vm_t *vm, sa_family_t family)
{
    if (unlikely(vm->ntries == vm->maxtries))
        vm_growtrie(vm);

    int idx = vm->ntries++;
    patinit(&vm->tries[idx], family);
    return idx;
}

extern stack_cell_t *vm_pop(filter_vm_t *vm);

extern void vm_push(filter_vm_t *vm, const stack_cell_t *cell);

extern void vm_pushaddr(filter_vm_t *vm, const netaddr_t *addr);

extern void vm_pushvalue(filter_vm_t *vm, int value);

static void withdrawn_left(filter_vm_t *vm)
{
    bgp_msg_t *msg = vm->bgp;
    if (unlikely(!msg))
        vm_abort(vm);

    netaddr_t *addr;
    startwithdrawn_r(msg);
    while ((addr = nextwithdrawn_r(msg)) != NULL)
        vm_pushaddr(vm, addr);

    if (unlikely(endwithdrawn_r(msg) != BGP_ENOERR))
        vm_abort(vm);
}

static void nlri_right(filter_vm_t *vm)
{
    bgp_msg_t *msg = vm->bgp;
    if (unlikely(!msg))
        vm_abort(vm);

    netaddr_t *addr;
    startnlri_r(msg);
    while ((addr = nextnlri_r(msg)) != NULL)
        vm_pushaddr(vm, addr);

    endnlri_r(msg);
}

static int parse_constant(filter_vm_t *vm, const char *tok, va_list va)
{
    int idx;

    if (tok[0] == '$') {
        const char *ptr = tok + 1;
        if (*ptr == '[')
            ptr++;

        if (!isdigit(*ptr))
            parsingerr("%s: illegal non-numeric register constant", tok);

        idx = atoi(ptr);
        do ptr++; while (isdigit(*ptr));

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

    } else {
        // IPv4 or IPv6 address
        idx = vm_newk(vm);
        if (stonaddr(&vm->kp[idx].addr, tok) != 0)
            parsingerr("invalid constant value '%s'", tok);
    }

    return idx;
}

static void compile_term(FILE *f, filter_vm_t *vm, int kind, va_list va)
{
    char *tok = expecttoken(f, NULL);
    if (strncasecmp(tok, "packet.", 7) == 0) {
        // packet accessor
        char *field = tok + 7;
        if (strcasecmp(field, "withdrawn") == 0) {
            bytecode_t opcode;
            if (kind == LEFT_TERM)
                opcode = pack_filter_op(FOPC_CALL, ACC_WITHDRAWN_LEFT);
            else
                opcode = pack_filter_op(FOPC_CALL, ACC_WITHDRAWN_RIGHT);

            vm_emit(vm, opcode);
        } else if (strcasecmp(field, "nlri") == 0) {
            bytecode_t opcode;
            if (kind == LEFT_TERM)
                opcode = pack_filter_op(FOPC_CALL, ACC_NLRI_LEFT);
            else
                opcode = pack_filter_op(FOPC_CALL, ACC_NLRI_RIGHT);

            vm_emit(vm, opcode);
        } else {
            parsingerr("unknown packet accessor '%s'", field);
        }
    } else if (strcmp(tok, "[") == 0) {
        // array/patricia
        if (kind == RIGHT_TERM) {
            // preload values in tries
            int v4 = vm_newtrie(vm, AF_INET);
            int v6 = vm_newtrie(vm, AF_INET6);

            vm_emit(vm, pack_filter_op(FOPC_SETTRIE,  v4));
            vm_emit(vm, pack_filter_op(FOPC_SETTRIE6, v6));
        }
        while (true) {
            tok = expecttoken(f, NULL);
            if (strcmp(tok, "]") == 0)
                break;

            int idx = parse_constant(vm, tok, va);
            vm_emit(vm, pack_filter_op(FOPC_LOADK, idx & 0xff));
            if (idx > 0xff)
                vm_emit(vm, pack_filter_op(FOPC_EXARG, idx >> 8));

            if (kind == RIGHT_TERM)
                vm_emit(vm, FOPC_STORE);
        }
    } else {
         // assume constant (actually single element array)
         if (kind == RIGHT_TERM) {
            // enable temporary tries
            vm_emit(vm, pack_filter_op(FOPC_SETTRIE,  0));
            vm_emit(vm, pack_filter_op(FOPC_SETTRIE6, 1));
        }

        int idx = parse_constant(vm, tok, va);
        vm_emit(vm, pack_filter_op(FOPC_LOADK, idx & 0xff));
        if (idx > 0xff)
            vm_emit(vm, pack_filter_op(FOPC_EXARG, idx >> 8));

        if (kind == RIGHT_TERM)
            vm_emit(vm, FOPC_STORE);
    }

    // XXX: would be nice to reuse code both for single constants and array...
}

static int compile_expr(FILE *f, filter_vm_t *vm, va_list va)
{
    char *tok = expecttoken(f, NULL);

    while (true) {
        if (strcasecmp(tok, "NOT") == 0) {
            compile_expr(f, vm, va);

            vm_emit(vm, FOPC_NOT);
        } else if (strcasecmp(tok, "(") == 0) {
            compile_expr(f, vm, va);
            expecttoken(f, ")");
        } else if (strcasecmp(tok, "CALL") == 0) {
            tok = expecttoken(f, NULL);
            // TODO parse function call
        } else {
            // TERM OP TERM
            ungettoken(tok);
            compile_term(f, vm, LEFT_TERM, va);

            tok = expecttoken(f, NULL);
            bytecode_t op;
            if (strcasecmp(tok, "IN") == 0) {
                op = FOPC_IN;
            } else if (strcasecmp(tok, "ALL") == 0) {
                op = FOPC_ALL;
            } else {
                parsingerr("unknown operation: '%s'", tok);
                break; // unreachable
            }

            compile_term(f, vm, RIGHT_TERM, va);

            vm_emit(vm, op);
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
            break;
        }
    }

    return 0;
}

void filter_init(filter_vm_t *vm)
{
    memset(vm, 0, sizeof(*vm));
    vm->sp    = vm->stackbuf;
    vm->kp    = vm->kbuf;
    vm->tries = vm->triebuf;
    vm->stacksiz = nelems(vm->stackbuf);
    vm->ksiz     = K_MAX + 1;  // reserved for user custom variables
    vm->maxk     = nelems(vm->kbuf);
    vm->ntries   = 2;  // 2 temporary tries
    vm->maxtries = nelems(vm->triebuf);
    vm->funcs[ACC_WITHDRAWN_LEFT] = withdrawn_left;
    vm->funcs[ACC_NLRI_RIGHT] = nlri_right;

    patinit(&vm->tries[0], AF_INET);
    patinit(&vm->tries[1], AF_INET6);
}

void filter_clear_error(void)
{
    err_msg[0] = '\0';
}

const char *filter_last_error(void)
{
    return err_msg;
}

static void handle_parse_error(const char *name, unsigned int lineno, const char *msg)
{
    (void) name, (void) lineno;

    strncpy(err_msg, msg, sizeof(err_msg) - 1);
    err_msg[sizeof(err_msg) - 1] = '\0';
    longjmp(err_jmp, -1);
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

static void exec_store(filter_vm_t *vm)
{
    stack_cell_t *cell = vm_pop(vm);
    netaddr_t *addr    = &cell->addr;
    switch (addr->family) {
    case AF_INET:
        if (!patinsertn(vm->curtrie, addr, NULL))
            vm_abort(vm);

        break;
    case AF_INET6:
        if (!patinsertn(vm->curtrie6, addr, NULL))
            vm_abort(vm);

        break;
    default:
        vm_abort(vm); // should never happen
    }
}

static void exec_in(filter_vm_t *vm)
{
    while (vm->si > 0) {
        stack_cell_t *cell = vm_pop(vm);
        netaddr_t *addr = &cell->addr;
        switch (addr->family) {
        case AF_INET:
            if (patsearchexactn(vm->curtrie, addr)) {
                vm_clearstack(vm);
                vm_pushvalue(vm, true);
                return;
            }
            break;
        case AF_INET6:
            if (patsearchexactn(vm->curtrie6, addr)) {
                vm_clearstack(vm);
                vm_pushvalue(vm, true);
                return;
            }
            break;
        default:
            vm_abort(vm);  // should never happen
        }
    }

    vm_pushvalue(vm, false);
}

static int filter_execute(filter_vm_t *vm, match_result_t *results, size_t *nresults)
{
    if (setjmp(vm->except) != 0)
        return -1;

    bytecode_t   *cp = vm->code;
    bytecode_t   *end = cp + vm->codesiz;
    stack_cell_t *cell;
    int opcode, arg, next_opcode;

    vm_clearstack(vm);
    while (cp < end) {
        opcode = *cp++;

        next_opcode = likely(cp < end) ? *cp : BAD_OPCODE;
        arg = opcode >> 8;
        if (unlikely((next_opcode & 0xff) == FOPC_EXARG)) {
            arg <<= 8;
            arg |= (next_opcode >> 8);
            cp++;
        }

        switch (opcode & 0xff) {
        case FOPC_NOP:
            break;
        case FOPC_LOAD:
            vm_pushvalue(vm, arg);
            break;
        case FOPC_LOADK:
            if (unlikely(arg >= vm->ksiz))
                vm_abort(vm);

            vm_push(vm, &vm->kp[arg]);
            break;
        case FOPC_STORE:
            exec_store(vm);
            break;
        case FOPC_NOT:
            cell = vm_pop(vm);
            vm_pushvalue(vm, !cell->value);
            break;
        case FOPC_CPASS:
            cell = vm_pop(vm);
            if (cell->value)
                return true;

            break;
        case FOPC_CFAIL:
            cell = vm_pop(vm);
            if (!cell->value)
                return false;

            break;
        case FOPC_CALL:
            vm->funcs[arg](vm);
            break;
        case FOPC_IN:
            exec_in(vm);
            break;
        case FOPC_SETTRIE:
            vm->curtrie = &vm->tries[arg];
            break;
        case FOPC_SETTRIE6:
            vm->curtrie6 = &vm->tries[arg];
            break;
        default:
            __builtin_unreachable();
        }
    }

    if (vm->si > 0) {
        cell = vm_pop(vm);
        return cell->value != 0;
    }

    return true;
}

int mrt_filter_r(mrt_msg_t *msg, filter_vm_t *vm, match_result_t *results, size_t *nresults)
{
    vm->bgp = NULL;
    vm->mrt = msg;

    return filter_execute(vm, results, nresults);
}

int mrt_filter(filter_vm_t *vm, match_result_t *results, size_t *nresults)
{
    return mrt_filter_r(getmrt(), vm, results, nresults);
}

int bgp_filter_r(bgp_msg_t *msg, filter_vm_t *vm, match_result_t *results, size_t *nresults)
{
    vm->bgp = msg;
    vm->mrt = NULL;
    return filter_execute(vm, results, nresults);
}

int bgp_filter(filter_vm_t *vm, match_result_t *results, size_t *nresults)
{
    return bgp_filter_r(getbgp(), vm, results, nresults);
}

