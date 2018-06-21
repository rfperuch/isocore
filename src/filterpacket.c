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

#include <isolario/filterintrin.h>
#include <isolario/filterpacket.h>
#include <isolario/parse.h>
#include <isolario/util.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

enum {
    K_GROW_STEP        = 32,
    STACK_GROW_STEP    = 128,
    HEAP_GROW_STEP     = 256,
    CODE_GROW_STEP     = 128,
    PATRICIA_GROW_STEP = 2
};

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
    free(vm->heap);
}

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

void vm_growcode(filter_vm_t *vm)
{
    unsigned short codesiz = vm->maxcode + CODE_GROW_STEP;
    bytecode_t *code = realloc(vm->code, codesiz * sizeof(*code));
    if (unlikely(!code))
        parsingerr("out of memory");  // FIXME: should *not* use parsingerr()...

    vm->code    = code;
    vm->maxcode = codesiz;
}

void vm_growk(filter_vm_t *vm)
{
    unsigned short ksiz = vm->maxk + K_GROW_STEP;
    stack_cell_t *k = NULL;
    if (k != vm->kbuf)
        k = vm->kp;

    k = realloc(k, ksiz * sizeof(*k));
    if (unlikely(!k))
        parsingerr("out of memory");  // FIXME

    if (vm->kp == vm->kbuf)
        memcpy(k, vm->kp, sizeof(vm->kbuf));

    vm->kp   = k;
    vm->maxk = ksiz;
}

void vm_growtrie(filter_vm_t *vm)
{
    patricia_trie_t *tries = NULL;
    if (vm->tries != vm->triebuf)
        tries = vm->tries;

    unsigned short ntries = vm->maxtries + PATRICIA_GROW_STEP;
    tries = realloc(tries, ntries * sizeof(*tries));
    if (unlikely(!tries))
        parsingerr("out of memory");  // FIXME

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

extern void vm_emit(filter_vm_t *vm, bytecode_t opcode);

/// @brief Reserve one constant (avoids the user custom section).
extern int vm_newk(filter_vm_t *vm);

extern int vm_newtrie(filter_vm_t *vm, sa_family_t family);

extern stack_cell_t *vm_peek(filter_vm_t *vm);

extern stack_cell_t *vm_pop(filter_vm_t *vm);

extern void vm_push(filter_vm_t *vm, const stack_cell_t *cell);

extern void vm_pushaddr(filter_vm_t *vm, const netaddr_t *addr);

extern void vm_pushvalue(filter_vm_t *vm, int value);

extern void vm_pushas(filter_vm_t *vm, uint32_t as);

extern void vm_exec_loadk(filter_vm_t *vm, int kidx);

extern void vm_check_array(filter_vm_t *vm, stack_cell_t *arr);

extern void *vm_heap_ptr(filter_vm_t *vm, unsigned int off);

void vm_exec_unpack(filter_vm_t *vm)
{
    stack_cell_t *cell = vm_pop(vm);
    vm_check_array(vm, cell);

    unsigned int off   = cell->base;
    unsigned int nels  = cell->nels;
    unsigned int elsiz = cell->elsiz;
    while (vm->stacksiz < vm->si + nels)
        vm_growstack(vm);

    const unsigned char *ptr = vm_heap_ptr(vm, off);
    for (unsigned int i = 0; i < nels; i++) {
        memcpy(&vm->sp[vm->si++], ptr, elsiz);
        ptr += elsiz;
    }
}

extern void vm_exec_store(filter_vm_t *vm);

extern void vm_exec_discard(filter_vm_t *vm);

extern void vm_exec_settrie(filter_vm_t *vm, int trie);

extern void vm_exec_settrie6(filter_vm_t *vm, int trie6);

extern void vm_exec_clrtrie(filter_vm_t *vm);

extern void vm_exec_clrtrie6(filter_vm_t *vm);

extern void vm_exec_ascmp(filter_vm_t *vm, int kidx);

extern void vm_exec_addrcmp(filter_vm_t *vm, int kidx);

extern void vm_exec_pfxcmp(filter_vm_t *vm, int kidx);

static void vm_require_bgp(filter_vm_t *vm)
{
    bgp_msg_t *msg = vm->bgp;
    if (unlikely(!msg))
        vm_abort(vm, VM_PACKET_MISMATCH);
}

static void vm_require_bgp_type(filter_vm_t *vm, int type)
{
    vm_require_bgp(vm);
    if (unlikely(getbgptype_r(vm->bgp) != type))
        vm_abort(vm, VM_PACKET_MISMATCH);
}

static void vm_accumulate_withdrawn(filter_vm_t *vm)
{
    netaddr_t *addr;
    while ((addr = nextwithdrawn_r(vm->bgp)) != NULL)
        vm_pushaddr(vm, addr);

    if (unlikely(endwithdrawn_r(vm->bgp) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

static void vm_accumulate_nlri(filter_vm_t *vm)
{
    netaddr_t *addr;
    while ((addr = nextnlri_r(vm->bgp)) != NULL) {
        vm_pushaddr(vm, addr);
        vm_exec_store(vm);
    }
    if (unlikely(endnlri_r(vm->bgp) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

static void vm_insert_withdrawn(filter_vm_t *vm)
{
    vm_require_bgp_type(vm, BGP_UPDATE);

    vm_exec_settrie(vm, VM_TMPTRIE);
    vm_exec_settrie6(vm, VM_TMPTRIE6);

    netaddr_t *addr;
    while ((addr = nextwithdrawn_r(vm->bgp)) != NULL) {
        vm_pushaddr(vm, addr);
        vm_exec_store(vm);
    }
    if (unlikely(endwithdrawn_r(vm->bgp) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

static void vm_insert_nlri(filter_vm_t *vm)
{
    vm_require_bgp_type(vm, BGP_UPDATE);

    vm_exec_settrie(vm, VM_TMPTRIE);
    vm_exec_settrie6(vm, VM_TMPTRIE6);

    netaddr_t *addr;
    while ((addr = nextwithdrawn_r(vm->bgp)) != NULL) {
        vm_pushaddr(vm, addr);
        vm_exec_store(vm);
    }
    if (unlikely(endwithdrawn_r(vm->bgp) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

void vm_exec_all_withdrawn_accumulate(filter_vm_t *vm)
{
    vm_require_bgp_type(vm, BGP_UPDATE);

    startallwithdrawn_r(vm->bgp);
    vm_accumulate_withdrawn(vm);
}

void vm_exec_withdrawn_accumulate(filter_vm_t *vm)
{
    vm_require_bgp_type(vm, BGP_UPDATE);

    startwithdrawn_r(vm->bgp);
    vm_accumulate_withdrawn(vm);
}

void vm_exec_all_withdrawn_insert(filter_vm_t *vm)
{
    vm_require_bgp_type(vm, BGP_UPDATE);

    startallwithdrawn_r(vm->bgp);
    vm_insert_withdrawn(vm);
}

void vm_exec_withdrawn_insert(filter_vm_t *vm)
{
    vm_require_bgp_type(vm, BGP_UPDATE);

    startwithdrawn_r(vm->bgp);
    vm_insert_withdrawn(vm);
}

void vm_exec_all_nlri_accumulate(filter_vm_t *vm)
{
    vm_require_bgp_type(vm, BGP_UPDATE);

    startallnlri_r(vm->bgp);
    vm_accumulate_nlri(vm);
}

void vm_exec_nlri_accumulate(filter_vm_t *vm)
{
    vm_require_bgp_type(vm, BGP_UPDATE);

    startnlri_r(vm->bgp);
    vm_accumulate_nlri(vm);
}

void vm_exec_all_nlri_insert(filter_vm_t *vm)
{
    vm_require_bgp_type(vm, BGP_UPDATE);

    startallnlri_r(vm->bgp);
    vm_insert_nlri(vm);
}

void vm_exec_nlri_insert(filter_vm_t *vm)
{
    vm_require_bgp_type(vm, BGP_UPDATE);

    startnlri_r(vm->bgp);
    vm_insert_nlri(vm);
}

void vm_emit_ex(filter_vm_t *vm, int opcode, int idx)
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

static void *vm_heap_ensure(filter_vm_t *vm, size_t aligned_size)
{
    size_t used = vm->highwater;
    used += vm->dynmarker;
    if (likely(vm->heapsiz - used >= aligned_size))
        return vm->heap;

    aligned_size += used;
    aligned_size += HEAP_GROW_STEP;
    void *heap = realloc(vm->heap, aligned_size);
    if (unlikely(!heap))
        return NULL;

    vm->heap    = heap;
    vm->heapsiz = aligned_size;
    return heap;
}

void *vm_heap_alloc(filter_vm_t *vm, size_t size, vm_heap_zone_t zone)
{
    // align allocation
    size += sizeof(max_align_t) - 1;
    size -= (size & (sizeof(max_align_t) - 1));

    unsigned char *ptr;

    if (unlikely(!vm_heap_ensure(vm, size)))
        return NULL;

    switch (zone) {
    case VM_HEAP_PERM:
        if (unlikely(vm->dynmarker > 0)) {
            assert(false);
            return NULL; // illegal!
        }

        ptr = (unsigned char *) vm->heap + vm->highwater;
        vm->highwater += size;
        return ptr;

    case VM_HEAP_TEMP:
        ptr = (unsigned char *) vm->heap + vm->highwater;
        ptr += vm->dynmarker;

        vm->dynmarker += size;
        return ptr;

    default:
        // should never happen
        return NULL;
    }
}

void filter_init(filter_vm_t *vm)
{
    memset(vm, 0, sizeof(*vm));
    vm->sp    = vm->stackbuf;
    vm->kp    = vm->kbuf;
    vm->tries = vm->triebuf;
    vm->stacksiz = nelems(vm->stackbuf);
    vm->ksiz     = KBASESIZ;  // reserved for known variables
    vm->maxk     = nelems(vm->kbuf);
    vm->ntries   = 2;  // 2 temporary tries
    vm->maxtries = nelems(vm->triebuf);
    vm->funcs[VM_WITHDRAWN_INSERT_FN]         = vm_exec_withdrawn_insert;
    vm->funcs[VM_WITHDRAWN_ACCUMULATE_FN]     = vm_exec_withdrawn_accumulate;
    vm->funcs[VM_ALL_WITHDRAWN_INSERT_FN]     = vm_exec_all_withdrawn_insert;
    vm->funcs[VM_ALL_WITHDRAWN_ACCUMULATE_FN] = vm_exec_all_withdrawn_accumulate;
    vm->funcs[VM_NLRI_INSERT_FN]              = vm_exec_nlri_insert;
    vm->funcs[VM_NLRI_ACCUMULATE_FN]          = vm_exec_nlri_accumulate;
    vm->funcs[VM_ALL_NLRI_INSERT_FN]          = vm_exec_all_nlri_insert;
    vm->funcs[VM_ALL_NLRI_ACCUMULATE_FN]      = vm_exec_all_nlri_accumulate;

    patinit(&vm->tries[VM_TMPTRIE],  AF_INET);
    patinit(&vm->tries[VM_TMPTRIE6], AF_INET6);
}

void vm_exec_exact(filter_vm_t *vm)
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

void vm_exec_subn(filter_vm_t *vm)
{
    while (vm->si > 0) {
        stack_cell_t *cell = vm_pop(vm);
        netaddr_t *addr = &cell->addr;
        switch (addr->family) {
        case AF_INET:
            if (patchecksubnetsofn(vm->curtrie, addr)) {
                vm_clearstack(vm);
                vm_pushvalue(vm, true);
                return;
            }
            break;
        case AF_INET6:
            if (patchecksubnetsofn(vm->curtrie6, addr)) {
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

void vm_exec_supern(filter_vm_t *vm)
{
    while (vm->si > 0) {
        stack_cell_t *cell = vm_pop(vm);
        netaddr_t *addr = &cell->addr;
        switch (addr->family) {
        case AF_INET:
            if (patchecksupernetsofn(vm->curtrie, addr)) {
                vm_clearstack(vm);
                vm_pushvalue(vm, true);
                return;
            }
            break;
        case AF_INET6:
            if (patchecksupernetsofn(vm->curtrie6, addr)) {
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


void vm_exec_reltd(filter_vm_t *vm)
{
    while (vm->si > 0) {
        stack_cell_t *cell = vm_pop(vm);
        netaddr_t *addr = &cell->addr;
        switch (addr->family) {
        case AF_INET:
            if (patcheckrelatedofn(vm->curtrie, addr)) {
                vm_clearstack(vm);
                vm_pushvalue(vm, true);
                return;
            }
            break;
        case AF_INET6:
            if (patcheckrelatedofn(vm->curtrie6, addr)) {
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

static int filter_execute(filter_vm_t *vm)
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

    vm->pc     = 0;
    vm->curblk = 0;
    vm_clearstack(vm);
    vm->dynmarker = 0;
    vm->error     = 0;
    exarg         = 0;

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

        EXECUTE(UNPACK):
            vm_exec_unpack(vm);
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
            PREDICT(CPASS);
            PREDICT(CFAIL);
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
                cell->value = 0;  // negate existing value
                if (vm->curblk == 0)
                    goto done;    // no more blocks, we're done

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

        EXECUTE(SUBN):
            vm_exec_subn(vm);
            PREDICT(CPASS);
            PREDICT(CFAIL);
            DISPATCH();

        EXECUTE(SUPERN):
            vm_exec_supern(vm);
            PREDICT(CPASS);
            PREDICT(CFAIL);
            DISPATCH();

        EXECUTE(RELTD):
            vm_exec_reltd(vm);
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

        EXECUTE(CLRTRIE):
            vm_exec_clrtrie(vm);
            DISPATCH();

        EXECUTE(CLRTRIE6):
            vm_exec_clrtrie6(vm);
            DISPATCH();

        EXECUTE(ADDRCMP):
            arg = vm_extendarg(vm_getarg(ip), exarg);
            vm_exec_addrcmp(vm, arg);
            exarg = 0;
            PREDICT(CPASS);
            PREDICT(CFAIL);
            PREDICT(NOT);
            DISPATCH();

        EXECUTE(PFXCMP):
            arg = vm_extendarg(vm_getarg(ip), exarg);
            vm_exec_pfxcmp(vm, arg);
            exarg = 0;
            PREDICT(CPASS);
            PREDICT(CFAIL);
            PREDICT(NOT);
            DISPATCH();

        EXECUTE(ASCMP):
            arg = vm_extendarg(vm_getarg(ip), exarg);
            vm_exec_ascmp(vm, arg);
            exarg = 0;
            PREDICT(CPASS);
            PREDICT(CFAIL);
            PREDICT(NOT);
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

int mrt_filter_r(mrt_msg_t *msg, filter_vm_t *vm)
{
    bgp_msg_t bgp;

    vm->bgp = NULL;
    vm->mrt = msg;

    if (isbgpwrapper_r(msg)) {
        // MRT wraps a BGP packet, extract it to enable BGP filtering
        void *data;
        size_t n;

        data = unwrapbgp4mp_r(msg, &n);
        setbgpread_r(&bgp, data, n, BGPF_NOCOPY);

        // populate feeder utility constants
        const bgp4mp_header_t *bgphdr = getbgp4mpheader();
        vm->kp[K_PEER_ADDR].addr = bgphdr->peer_addr;
        vm->kp[K_PEER_AS].as     = bgphdr->peer_as;

        vm->bgp = &bgp;
    }

    int result = filter_execute(vm);
    if (vm->bgp)
        bgpclose_r(vm->bgp);

    return result;
}

int mrt_filter(filter_vm_t *vm)
{
    return mrt_filter_r(getmrt(), vm);
}

int bgp_filter_r(bgp_msg_t *msg, filter_vm_t *vm)
{
    vm->bgp = msg;
    vm->mrt = NULL;
    return filter_execute(vm);
}

int bgp_filter(filter_vm_t *vm)
{
    return bgp_filter_r(getbgp(), vm);
}
