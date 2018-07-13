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
#include <isolario/parse.h>
#include <stdlib.h>

enum {
    K_GROW_STEP        = 32,
    STACK_GROW_STEP    = 128,
    HEAP_GROW_STEP     = 256,
    CODE_GROW_STEP     = 128,
    PATRICIA_GROW_STEP = 2
};

void vm_growstack(filter_vm_t *vm)
{
    stack_cell_t *stk = NULL;
    if (vm->sp != vm->stackbuf)
        stk = vm->sp;

    unsigned short stacksiz = vm->stacksiz + STACK_GROW_STEP;
    stk = realloc(stk, stacksiz * sizeof(*stk));
    if (unlikely(!stk))
        vm_abort(vm, VM_STACK_OVERFLOW);
    if (vm->sp == vm->stackbuf)
        memcpy(stk, vm->stackbuf, sizeof(vm->stackbuf));

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

extern void vm_exec_not(filter_vm_t *vm);

extern void vm_exec_store(filter_vm_t *vm);

extern void vm_exec_discard(filter_vm_t *vm);

extern void vm_exec_settrie(filter_vm_t *vm, int trie);

extern void vm_exec_settrie6(filter_vm_t *vm, int trie6);

extern void vm_exec_clrtrie(filter_vm_t *vm);

extern void vm_exec_clrtrie6(filter_vm_t *vm);

extern void vm_exec_ascmp(filter_vm_t *vm, int kidx);

extern void vm_exec_addrcmp(filter_vm_t *vm, int kidx);

extern void vm_exec_pfxcmp(filter_vm_t *vm, int kidx);

extern void vm_exec_all_withdrawn_insert(filter_vm_t *vm);
extern void vm_exec_all_withdrawn_accumulate(filter_vm_t *vm);
extern void vm_exec_withdrawn_accumulate(filter_vm_t *vm);
extern void vm_exec_withdrawn_insert(filter_vm_t *vm);

extern void vm_exec_all_nlri_insert(filter_vm_t *vm);
extern void vm_exec_all_nlri_accumulate(filter_vm_t *vm);
extern void vm_exec_nlri_insert(filter_vm_t *vm);
extern void vm_exec_nlri_accumulate(filter_vm_t *vm);

extern void vm_exec_exact(filter_vm_t *vm);
extern void vm_exec_subnet(filter_vm_t *vm);
extern void vm_exec_supernet(filter_vm_t *vm);
extern void vm_exec_related(filter_vm_t *vm);

extern void vm_exec_pfxcontains(filter_vm_t *vm, int kidx);

extern void vm_exec_addrcontains(filter_vm_t *vm, int kidx);

extern void vm_exec_ascontains(filter_vm_t *vm, int kidx);

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
        assert(false);
        return NULL;
    }
}