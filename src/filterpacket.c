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

extern char *filter_strerror(int err);

void filter_destroy(filter_vm_t *vm)
{
    for (unsigned int i = 0; i < vm->ntries; i++)
        patdestroy(&vm->tries[i]);

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
        vm_abort(vm, VM_STACK_OVERFLOW);

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

    if (vm->kp == vm->kbuf)
        memcpy(k, vm->kp, sizeof(vm->kbuf));

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

    if (vm->tries == vm->triebuf)
        memcpy(tries, vm->tries, sizeof(vm->triebuf));

    vm->tries    = tries;
    vm->maxtries = ntries;
}

extern bytecode_t vm_makeop(int opcode, int arg);

extern int vm_getopcode(bytecode_t code);

extern int vm_getarg(bytecode_t code);

extern int vm_extendarg(int arg, int exarg);

extern void vm_clearstack(filter_vm_t *vm);

extern noreturn void vm_abort(filter_vm_t *vm, int error);

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

extern stack_cell_t *vm_peek(filter_vm_t *vm);

extern stack_cell_t *vm_pop(filter_vm_t *vm);

extern void vm_push(filter_vm_t *vm, const stack_cell_t *cell);

extern void vm_pushaddr(filter_vm_t *vm, const netaddr_t *addr);

extern void vm_pushvalue(filter_vm_t *vm, int value);

extern void vm_exec_loadk(filter_vm_t *vm, int kidx);

extern void vm_exec_store(filter_vm_t *vm);

extern void vm_exec_discard(filter_vm_t *vm);

extern void vm_exec_settrie(filter_vm_t *vm, int trie);

extern void vm_exec_settrie6(filter_vm_t *vm, int trie6);

extern void vm_exec_clrtrie(filter_vm_t *vm);

extern void vm_exec_clrtrie6(filter_vm_t *vm);

void vm_exec_withdrawn_accumulate(filter_vm_t *vm)
{
    bgp_msg_t *msg = vm->bgp;
    if (unlikely(!msg))
        vm_abort(vm, VM_PACKET_MISMATCH);

    netaddr_t *addr;
    startwithdrawn_r(msg);
    while ((addr = nextwithdrawn_r(msg)) != NULL)
        vm_pushaddr(vm, addr);

    if (unlikely(endwithdrawn_r(msg) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

void vm_exec_withdrawn_insert(filter_vm_t *vm)
{
    bgp_msg_t *msg = vm->bgp;
    if (unlikely(!msg))
        vm_abort(vm, VM_PACKET_MISMATCH);

    vm_exec_settrie(vm, VM_TMPTRIE);
    vm_exec_settrie6(vm, VM_TMPTRIE6);

    netaddr_t *addr;
    startwithdrawn_r(msg);
    while ((addr = nextwithdrawn_r(msg)) != NULL) {
        vm_pushaddr(vm, addr);
        vm_exec_store(vm);
    }
    if (unlikely(endwithdrawn_r(msg) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

void vm_exec_nlri_accumulate(filter_vm_t *vm)
{
    bgp_msg_t *msg = vm->bgp;
    if (unlikely(!msg))
        vm_abort(vm, VM_PACKET_MISMATCH);

    netaddr_t *addr;
    startnlri_r(msg);
    while ((addr = nextnlri_r(msg)) != NULL)
        vm_pushaddr(vm, addr);

    if (unlikely(endnlri_r(msg) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

void vm_exec_nlri_insert(filter_vm_t *vm)
{
    bgp_msg_t *msg = vm->bgp;
    if (unlikely(!msg))
        vm_abort(vm, VM_PACKET_MISMATCH);

    vm_exec_settrie(vm, VM_TMPTRIE);
    vm_exec_settrie6(vm, VM_TMPTRIE6);

    netaddr_t *addr;
    startnlri_r(msg);
    while ((addr = nextnlri_r(msg)) != NULL) {
        vm_pushaddr(vm, addr);
        vm_exec_store(vm);
    }
    if (unlikely(endnlri_r(msg) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

static void vm_emit_indexed(filter_vm_t *vm, int opcode, int idx)
{
    // NOTE: paranoid, assumes 8 bit bytes... as the whole Isolario project does

    // find the most significant byte
    int msb = (sizeof(idx) - 1) * 8;
    while (msb > 0) {
        if ((idx >> msb) & 0xff)
            break;

        msb -= 8;
    }

    // emit value most-significant byte first
    while (msb > 0) {
        vm_emit(vm, vm_makeop(FOPC_EXARG, (idx >> msb) & 0xff));
        msb -= 8;
    }
    // emit the instruction with the least-signficiant byte
    vm_emit(vm, vm_makeop(opcode, idx & 0xff));
}

/// @brief Handes "$constant" and "$[constant]" tokens
static int parse_registry(const char *tok, va_list va)
{
    const char *ptr = tok + 1;
    if (*ptr == '[')
        ptr++;

    if (!isdigit(*ptr))
        parsingerr("%s: illegal non-numeric register constant", tok);

    int idx = atoi(ptr);
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

        vm_emit_indexed(vm, FOPC_SETTRIE, v4);
        vm_emit_indexed(vm, FOPC_SETTRIE6, v6);
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
        } else if (strcasecmp(field, "nlri") == 0) {
            bytecode_t opcode;
            if (kind == LEFT_TERM)
                opcode = vm_makeop(FOPC_CALL, VM_NLRI_ACCUMULATE_FN);
            else
                opcode = vm_makeop(FOPC_CALL, VM_NLRI_INSERT_FN);

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
            vm_emit_indexed(vm, FOPC_CALL, idx);
        } else {
            // TERM OP TERM
            ungettoken(tok);
            compile_term(f, vm, LEFT_TERM, va);

            tok = expecttoken(f, NULL);
            bytecode_t op;
            if (strcasecmp(tok, "EXACT") == 0) {
                op = FOPC_EXACT;
            } else if (strcasecmp(tok, "ANY") == 0) {
                op = FOPC_ANY;
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
    vm->funcs[VM_WITHDRAWN_INSERT_FN]     = vm_exec_withdrawn_insert;
    vm->funcs[VM_WITHDRAWN_ACCUMULATE_FN] = vm_exec_withdrawn_accumulate;
    vm->funcs[VM_NLRI_INSERT_FN]     = vm_exec_nlri_insert;
    vm->funcs[VM_NLRI_ACCUMULATE_FN] = vm_exec_nlri_accumulate;

    vm->flags = 0;

    patinit(&vm->tries[VM_TMPTRIE],  AF_INET);
    patinit(&vm->tries[VM_TMPTRIE6], AF_INET6);
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

void vm_exec_exact(filter_vm_t *vm)
{
    while (vm->si > 0) {
        stack_cell_t *cell = vm_pop(vm);
        netaddr_t *addr = &cell->addr;
        switch (addr->family) {
        case AF_INET:
            if (!patsearchexactn(vm->curtrie, addr)) {
                vm_clearstack(vm);
                vm_pushvalue(vm, false);
                return;
            }
            break;
        case AF_INET6:
            if (!patsearchexactn(vm->curtrie6, addr)) {
                vm_clearstack(vm);
                vm_pushvalue(vm, false);
                return;
            }
            break;
        default:
            vm_abort(vm, VM_SURPRISING_BYTES);  // should never happen
            break;
        }
    }

    vm_pushvalue(vm, true);
}

void vm_exec_any(filter_vm_t *vm)
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
            vm_abort(vm, VM_SURPRISING_BYTES);  // should never happen
            break;
        }
    }

    vm_pushvalue(vm, false);

}

static int filter_execute(filter_vm_t *vm, match_result_t *results, size_t *nresults)
{
// disable pedantic diagnostic about taking address label
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

#if defined(__GNUC__) && !defined(ISOLARIO_VM_PLAIN_SWITCH)
// use computed goto to speed-up bytecode interpreter
#include "vm_opcodes.h"

#define FETCH() ip = vm->code[vm->pc++];                                           \
                np = likely(vm->pc < vm->codesiz) ? vm->code[vm->pc] : BAD_OPCODE; \
                goto *vm_opcode_table[vm_getopcode(ip)]

#define PREDICT(opcode) do {                                                     \
        if (likely(vm_getopcode(np) == FOPC_##opcode)) {                         \
            ip = np;                                                             \
            np = likely(vm->pc < vm->codesiz) ? vm->code[++vm->pc] : BAD_OPCODE; \
            goto *vm_opcode_table[FOPC_##opcode];                                \
        }                                                                        \
    } while (false)

#define DISPATCH() break

#define EXECUTE(opcode) case FOPC_##opcode: \
                        EX_##opcode

#define EXECUTE_SIGILL default: \
                       EX_SIGILL

#else
// portable C

#define FETCH() ip = vm->code[vm->pc++];                                           \
                np = likely(vm->pc < vm->codesiz) ? vm->code[vm->pc] : BAD_OPCODE;

#define PREDICT(opcode) do (void) 0; while (false)

#define DISPATCH() break

#define EXECUTE(opcode) case FOPC_##opcode

#define EXECUTE_SIGILL  default
#endif

    if (setjmp(vm->except) != 0)
        return vm->error;

    bytecode_t ip, np;
    int arg, exarg;
    stack_cell_t *cell;

    vm->curblk = 0;
    vm_clearstack(vm);
    vm->error = 0;
    exarg = 0;

    vm_exec_settrie(vm, VM_TMPTRIE);
    vm_exec_settrie6(vm, VM_TMPTRIE6);
    vm_exec_clrtrie(vm);
    vm_exec_clrtrie6(vm);

    while (vm->pc < vm->codesiz) {
        FETCH();

        switch (vm_getopcode(ip)) {
        EXECUTE(NOP):
            DISPATCH();

        EXECUTE(BLK):
            arg = vm_extendarg(vm_getarg(ip), exarg);
            if (unlikely(vm->pc + arg > vm->codesiz))
                vm_abort(vm, VM_BAD_BLOCK);
            if (unlikely(vm->curblk == nelems(vm->blkstack)))
                vm_abort(vm, VM_BLOCKS_OVERFLOW);

            vm->blkstack[vm->curblk++] = vm->pc + arg;
            exarg = 0;
            DISPATCH();

        EXECUTE(LOAD):
            vm_pushvalue(vm, vm_extendarg(vm_getarg(ip), exarg));
            exarg = 0;
            DISPATCH();

        EXECUTE(LOADK):
            arg = vm_extendarg(vm_getarg(ip), exarg);
            vm_exec_loadk(vm, arg);
            exarg = 0;
            DISPATCH();

        EXECUTE(EXARG):
            arg = vm_getarg(ip);
            exarg <<= 8;
            exarg  |= arg;

            PREDICT(LOADK);
            PREDICT(BLK);
            PREDICT(LOAD);
            PREDICT(CALL);

            DISPATCH();

        EXECUTE(STORE):
            vm_exec_store(vm);
            DISPATCH();

        EXECUTE(DISCARD):
            vm_exec_discard(vm);
            DISPATCH();

        EXECUTE(NOT):
            cell = vm_pop(vm);
            vm_pushvalue(vm, !cell->value);
            DISPATCH();

        EXECUTE(CPASS):
            cell = vm_peek(vm);
            if (cell->value) {
                if (vm->curblk == 0)
                    goto done; // no more blocks, we're done

                // pop one block and go on
                vm->pc = vm->blkstack[--vm->curblk];
            } else {
                vm_pop(vm);  // discard and proceed
            }
            DISPATCH();

        EXECUTE(CFAIL):
            cell = vm_peek(vm);
            if (cell->value) {
                if (vm->curblk == 0)
                    goto done;  // no more blocks, we're done

                // pop one block and see what happens
                vm->pc = vm->blkstack[--vm->curblk];
            } else {
                vm_pop(vm);  // discard and proceed
            }
            DISPATCH();

        EXECUTE(CALL):
            arg = vm_extendarg(vm_getarg(ip), exarg);
            if (unlikely(arg >= VM_FUNCS_COUNT))
                vm_abort(vm, VM_FUNC_UNDEFINED);
            if (unlikely(!vm->funcs[arg]))
                vm_abort(vm, VM_FUNC_UNDEFINED);

            vm->funcs[arg](vm);
            exarg = 0;
            DISPATCH();

        EXECUTE(EXACT):
            vm_exec_exact(vm);

            PREDICT(CPASS);
            PREDICT(CFAIL);
            DISPATCH();

        EXECUTE(ANY):
            vm_exec_any(vm);

            PREDICT(CPASS);
            PREDICT(CFAIL);
            DISPATCH();

        EXECUTE(SETTRIE):
            arg = vm_extendarg(vm_getarg(ip), exarg);
            vm_exec_settrie(vm, arg);
            exarg = 0;
            DISPATCH();

        EXECUTE(SETTRIE6):
            arg = vm_extendarg(vm_getarg(ip), exarg);
            vm_exec_settrie6(vm, arg);
            exarg = 0;
            DISPATCH();

        EXECUTE(SUBNET):
            // TODO
            DISPATCH();

        EXECUTE(CLRTRIE):
            vm_exec_clrtrie(vm);
            DISPATCH();

        EXECUTE(CLRTRIE6):
            vm_exec_clrtrie6(vm);
            DISPATCH();

        EXECUTE_SIGILL:
            vm_abort(vm, VM_ILLEGAL_OPCODE);
            DISPATCH();  // unreachable
        }
    }

done:
    cell = vm_pop(vm);
    return cell->value != 0;

#undef FETCH
#undef DISPATCH
#undef PREDICT
#undef EXECUTE
#undef EXECUTE_SIGILL
#pragma GCC diagnostic pop
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

