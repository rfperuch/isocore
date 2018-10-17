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

/**
 * @file isolario/filterintrin.h
 *
 * @brief Filter Virtual Machine intrinsics, low level access to the packet
 *        filtering engine.
 */

#ifndef ISOLARIO_FILTERINTRIN_H_
#define ISOLARIO_FILTERINTRIN_H_

#include <assert.h>
#include <isolario/netaddr.h>
#include <isolario/filterpacket.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <string.h>

/// @brief Filter Virtual Machine opcodes.
enum {
    BAD_OPCODE = -1,

    FOPC_NOP,      ///< NOP: No operation, does nothing.

    FOPC_BLK,      ///< NONE: push a new block in block stack.
    FOPC_ENDBLK,   ///< NONE: mark the end of the latest opened block.
    FOPC_LOAD,     ///< PUSH: direct value load
    FOPC_LOADK,    ///< PUSH: load from constants environment
    FOPC_UNPACK,   ///< POP-PUSH: unpack an array constant into stack.
    FOPC_EXARG,    ///< NOP: extends previous operation's argument range
    FOPC_STORE,    ///< POP: store address into current trie (v4 or v6 depends on address)
    FOPC_DISCARD,  ///< POP: remove address from current trie (v4 or v6 depends on address)
    FOPC_NOT,      ///< POP-PUSH: pops stack topmost value, negates it and pushes it back.
    FOPC_CPASS,    ///< POP: pops topmost stack element and terminates with PASS if value is true.
    FOPC_CFAIL,    ///< POP: pops topmost stack element and terminates with FAIL if value is false.

    FOPC_SETTLE,   ///< NONE: forcefully close an iteration sequence

    FOPC_HASATTR,  ///< HASATTR: push true if attribute is present, false otherwise.

    FOPC_EXACT,
        /**<
         * Pops the entire stack and verifies that at least one *address* has an *exact* relationship with
         * the addresses stored inside the current tries, pushes a boolean result.
         *
         * This opcode expects that the entire stack is composed of cells containing \a netaddr_t.
         *
         * @note Stack operation mode is POPA-PUSH, this opcode has an announce/withdrawn accessor argument.
         */

    FOPC_SUBNET,
    FOPC_SUPERNET,
    FOPC_RELATED,

    FOPC_PFXCONTAINS,    ///< POPA-PUSH: Prefix Contained
    FOPC_ADDRCONTAINS,
    FOPC_ASCONTAINS,

    FOPC_ASPMATCH,
        /**<
         * Pops the entire stack and verifies that each AS in the stack
         * appears within the PATH field identified by this instruction argument.
         *
         * This opcode expects that the entire stack is composed of cells containing \a wide_as_t.
         *
         * @note Stack operation mode is POPA-PUSH, this opcode has an AS PATH accessor argument.
         */

    FOPC_ASPSTARTS,
    FOPC_ASPENDS,
    FOPC_ASPEXACT,

    FOPC_CALL,     ///< ???: call a function
    FOPC_SETTRIE,
    FOPC_SETTRIE6,
    FOPC_CLRTRIE,
    FOPC_CLRTRIE6,
    FOPC_PFXCMP,
    FOPC_ADDRCMP,
    FOPC_ASCMP,

    OPCODES_COUNT
};

// operator accessors (8 bits)
enum {
    FOPC_ACCESS_SETTLE        = 1 << 7,  // General flag to rewind the iterator (equivalent to call settle from the beginning)

    // NLRI/Withdrawn accessors
    FOPC_ACCESS_NLRI          = 1 << 0,
    FOPC_ACCESS_WITHDRAWN     = 1 << 1,
    FOPC_ACCESS_ALL           = 1 << 2,

    // AS access
    FOPC_ACCESS_AS_PATH       = 1 << 0,
    FOPC_ACCESS_AS4_PATH      = 1 << 1,
    FOPC_ACCESS_REAL_AS_PATH  = 1 << 2
};

void vm_growstack(filter_vm_t *vm);
void vm_growcode(filter_vm_t *vm);
void vm_growk(filter_vm_t *vm);
void vm_growtrie(filter_vm_t *vm);

inline noreturn void vm_abort(filter_vm_t *vm, int error)
{
    vm->error = error;
    longjmp(vm->except, -1);
}

inline bytecode_t vm_makeop(int opcode, int arg)
{
    return ((arg << 8) & 0xff00) | (opcode & 0xff);
}

inline int vm_getopcode(bytecode_t code)
{
    return code & 0xff;
}

inline int vm_getarg(bytecode_t code)
{
    return code >> 8;
}

inline int vm_extendarg(int arg, int exarg)
{
    return ((exarg << 8) | arg) & 0x7fffffff;
}

/// @brief Reserve one constant (avoids the user custom section).
inline int vm_newk(filter_vm_t *vm)
{
    if (unlikely(vm->ksiz == vm->maxk))
        vm_growk(vm);

    return vm->ksiz++;
}

inline int vm_newtrie(filter_vm_t *vm, sa_family_t family)
{
    if (unlikely(vm->ntries == vm->maxtries))
        vm_growtrie(vm);

    int idx = vm->ntries++;
    patinit(&vm->tries[idx], family);
    return idx;
}

/// @brief Emit one bytecode operation
inline void vm_emit(filter_vm_t *vm, bytecode_t opcode)
{
    if (unlikely(vm->codesiz == vm->maxcode))
        vm_growcode(vm);

    vm->code[vm->codesiz++] = opcode;
}

void vm_emit_ex(filter_vm_t *vm, int opcode, int idx);

// Virtual Machine dynamic memory:

typedef enum { VM_HEAP_PERM, VM_HEAP_TEMP } vm_heap_zone_t;

enum { VM_BAD_HEAP_PTR = -1 };

intptr_t vm_heap_alloc(filter_vm_t *vm, size_t size, vm_heap_zone_t zone);

/// @warning Only valid for the last allocated VM_HEAP_TEMP chunk!!!
inline void vm_heap_return(filter_vm_t* vm, size_t size)
{
    // align allocation
    size += sizeof(max_align_t) - 1;
    size -= (size & (sizeof(max_align_t) - 1));

    assert(vm->dynmarker >= size);

    vm->dynmarker -= size;
}

/// @warning Only valid for the last allocated VM_HEAP_TEMP chunk!!!
intptr_t vm_heap_grow(filter_vm_t *vm, intptr_t addr, size_t newsize);


// General Virtual Machine operations:

inline void vm_clearstack(filter_vm_t *vm)
{
    vm->si = 0;
}

inline stack_cell_t *vm_peek(filter_vm_t *vm)
{
    if (unlikely(vm->si == 0))
        vm_abort(vm, VM_STACK_UNDERFLOW);

    return &vm->sp[vm->si - 1];
}

inline stack_cell_t *vm_pop(filter_vm_t *vm)
{
    if (unlikely(vm->si == 0))
        vm_abort(vm, VM_STACK_UNDERFLOW);

    return &vm->sp[--vm->si];
}

inline void vm_push(filter_vm_t *vm, const stack_cell_t *cell)
{
    if (unlikely(vm->si == vm->stacksiz))
        vm_growstack(vm);

    memcpy(&vm->sp[vm->si++], cell, sizeof(*cell));
}

inline void vm_pushaddr(filter_vm_t *vm, const netaddr_t *addr)
{
    // especially optimized, since it's very common
    if (unlikely(vm->si == vm->stacksiz))
        vm_growstack(vm);

    memcpy(&vm->sp[vm->si++].addr, addr, sizeof(*addr));
}

inline void vm_pushvalue(filter_vm_t *vm, int value)
{
    // optimized, since it's common
    if (unlikely(vm->si == vm->stacksiz))
        vm_growstack(vm);

    vm->sp[vm->si++].value = value;
}

inline void vm_pushas(filter_vm_t *vm, wide_as_t as)
{
    if (unlikely(vm->si == vm->stacksiz))
        vm_growstack(vm);

    vm->sp[vm->si++].as  = as;
}

inline void vm_exec_loadk(filter_vm_t *vm, int kidx)
{
    if (unlikely(kidx >= vm->ksiz))
        vm_abort(vm, VM_K_UNDEFINED);

    vm_push(vm, &vm->kp[kidx]);
}

/// @brief Breaks from the current BLK, leaves vm->pc at the corresponding ENDBLK,
///        vm->pc is assumed to be inside the BLK
inline void vm_exec_break(filter_vm_t *vm)
{
    int nblk = 1; // encountered blocks

    while (vm->pc < vm->codesiz) {
        if (vm->code[vm->pc] == FOPC_ENDBLK)
            nblk--;
        if (vm->code[vm->pc] == FOPC_BLK)
            nblk++;

        if (nblk == 0)
            break;

        vm->pc++;
    }
}

inline void vm_exec_not(filter_vm_t *vm)
{
    stack_cell_t *cell = vm_peek(vm);
    cell->value = !cell->value;
}

inline void *vm_heap_ptr(filter_vm_t *vm, intptr_t off)
{
    assert(off >= 0);

    return (unsigned char *) vm->heap + off;  // duh...
}

inline void vm_check_array(filter_vm_t *vm, stack_cell_t *arr)
{
    size_t bound = arr->base;
    bound += arr->nels * arr->elsiz;

    if (unlikely(arr->elsiz > sizeof(stack_cell_t) || bound > vm->heapsiz))
        vm_abort(vm, VM_BAD_ARRAY);
}

void vm_exec_unpack(filter_vm_t *vm);

inline void vm_exec_store(filter_vm_t *vm)
{
    stack_cell_t *cell    = vm_pop(vm);
    netaddr_t *addr       = &cell->addr;
    patricia_trie_t *trie = vm->curtrie;
    switch (addr->family) {
    case AF_INET6:
        trie = vm->curtrie6;
        // fallthrough
    case AF_INET:
        if (!patinsertn(trie, addr, NULL))
            vm_abort(vm, VM_OUT_OF_MEMORY);

        break;
    default:
        vm_abort(vm, VM_SURPRISING_BYTES); // should never happen
        break;
    }
}

inline void vm_exec_discard(filter_vm_t *vm)
{
    stack_cell_t *cell    = vm_pop(vm);
    netaddr_t *addr       = &cell->addr;
    patricia_trie_t *trie = vm->curtrie;
    switch (addr->family) {
    case AF_INET6:
        trie = vm->curtrie6;
        // fallthrough
    case AF_INET:
        patremoven(trie, addr);
        break;
    default:
        vm_abort(vm, VM_SURPRISING_BYTES);  // should never happen
        break;
    }
}

inline void vm_exec_clrtrie(filter_vm_t *vm)
{
    patclear(vm->curtrie);
}

inline void vm_exec_clrtrie6(filter_vm_t *vm)
{
    patclear(vm->curtrie6);
}

inline void vm_exec_settrie(filter_vm_t *vm, int trie)
{
    if (unlikely((unsigned int) trie >= vm->ntries))
        vm_abort(vm, VM_TRIE_UNDEFINED);

    vm->curtrie = &vm->tries[trie];
    if (unlikely(vm->curtrie->maxbitlen != 32))
        vm_abort(vm, VM_TRIE_MISMATCH);
}

inline void vm_exec_settrie6(filter_vm_t *vm, int trie6)
{
    if (unlikely((unsigned int) trie6 >= vm->ntries))
        vm_abort(vm, VM_TRIE_UNDEFINED);

    vm->curtrie6 = &vm->tries[trie6];
    if (unlikely(vm->curtrie6->maxbitlen != 128))
        vm_abort(vm, VM_TRIE_MISMATCH);
}

inline void vm_exec_ascmp(filter_vm_t *vm, int kidx)
{
    if (unlikely((unsigned int) kidx >= vm->ksiz))
        vm_abort(vm, VM_K_UNDEFINED);

    stack_cell_t *a = vm_peek(vm);
    stack_cell_t *b = &vm->kp[kidx];

    a->value = (a->as == b->as);
}

inline void vm_exec_addrcmp(filter_vm_t *vm, int kidx)
{
    if (unlikely((unsigned int) kidx >= vm->ksiz))
        vm_abort(vm, VM_K_UNDEFINED);

    stack_cell_t *a = vm_peek(vm);
    stack_cell_t *b = &vm->kp[kidx];
    a->value = naddreq(&a->addr, &b->addr);
}

inline void vm_exec_pfxcmp(filter_vm_t *vm, int kidx)
{
    if (unlikely((unsigned int) kidx >= vm->ksiz))
        vm_abort(vm, VM_K_UNDEFINED);

    stack_cell_t *a = vm_peek(vm);
    stack_cell_t *b = &vm->kp[kidx];
    a->value = prefixeq(&a->addr, &b->addr);
}

inline void vm_exec_settle(filter_vm_t *vm)
{
    if (vm->settle_func) {
        vm->settle_func(vm->bgp);
        vm->settle_func = NULL;

        vm->access_mask = 0;
    }
}

void vm_exec_hasattr(filter_vm_t *vm, int code);

inline void vm_exec_all_withdrawn_insert(filter_vm_t *vm)
{
    if (unlikely(getbgptype_r(vm->bgp) != BGP_UPDATE))
        vm_abort(vm, VM_PACKET_MISMATCH);

    startallwithdrawn_r(vm->bgp);

    const netaddr_t *addr;
    while ((addr = nextwithdrawn_r(vm->bgp)) != NULL) {
        patricia_trie_t *trie = vm->curtrie;
        switch (addr->family) {
        case AF_INET6:
            trie = vm->curtrie6;
            // fallthrough
        case AF_INET:
            if (!patinsertn(trie, addr, NULL))
                vm_abort(vm, VM_OUT_OF_MEMORY);

            break;
        default:
            vm_abort(vm, VM_SURPRISING_BYTES); // should never happen
            break;
        }
    }

    if (unlikely(endwithdrawn_r(vm->bgp) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

inline void vm_exec_all_withdrawn_accumulate(filter_vm_t *vm)
{
    if (unlikely(getbgptype_r(vm->bgp) != BGP_UPDATE))
        vm_abort(vm, VM_PACKET_MISMATCH);

    startallwithdrawn_r(vm->bgp);

    const netaddr_t *addr;
    while ((addr = nextwithdrawn_r(vm->bgp)) != NULL)
        vm_pushaddr(vm, addr);

    if (unlikely(endwithdrawn_r(vm->bgp) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

inline void vm_exec_withdrawn_accumulate(filter_vm_t *vm)
{
    if (unlikely(getbgptype_r(vm->bgp) != BGP_UPDATE))
        vm_abort(vm, VM_PACKET_MISMATCH);

    startwithdrawn_r(vm->bgp);

    const netaddr_t *addr;
    while ((addr = nextwithdrawn_r(vm->bgp)) != NULL)
        vm_pushaddr(vm, addr);

    if (unlikely(endwithdrawn_r(vm->bgp) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

inline void vm_exec_withdrawn_insert(filter_vm_t *vm)
{
    if (unlikely(getbgptype_r(vm->bgp) != BGP_UPDATE))
        vm_abort(vm, VM_PACKET_MISMATCH);

    startwithdrawn_r(vm->bgp);

    const netaddr_t *addr;
    while ((addr = nextwithdrawn_r(vm->bgp)) != NULL) {
        patricia_trie_t *trie = vm->curtrie;
        switch (addr->family) {
        case AF_INET6:
            trie = vm->curtrie6;
            // fallthrough
        case AF_INET:
            if (!patinsertn(trie, addr, NULL))
                vm_abort(vm, VM_OUT_OF_MEMORY);

            break;
        default:
            vm_abort(vm, VM_SURPRISING_BYTES); // should never happen
            break;
        }
    }

    if (unlikely(endwithdrawn_r(vm->bgp) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

inline void vm_exec_all_nlri_insert(filter_vm_t *vm)
{
    if (unlikely(getbgptype_r(vm->bgp) != BGP_UPDATE))
        vm_abort(vm, VM_PACKET_MISMATCH);

    startallnlri_r(vm->bgp);

    const netaddr_t *addr;
    while ((addr = nextnlri_r(vm->bgp)) != NULL) {
        patricia_trie_t *trie = vm->curtrie;
        switch (addr->family) {
        case AF_INET6:
            trie = vm->curtrie6;
            // fallthrough
        case AF_INET:
            if (!patinsertn(trie, addr, NULL))
                vm_abort(vm, VM_OUT_OF_MEMORY);

            break;
        default:
            vm_abort(vm, VM_SURPRISING_BYTES); // should never happen
            break;
        }
    }

    if (unlikely(endnlri_r(vm->bgp) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

inline void vm_exec_all_nlri_accumulate(filter_vm_t *vm)
{
    if (unlikely(getbgptype_r(vm->bgp) != BGP_UPDATE))
        vm_abort(vm, VM_PACKET_MISMATCH);

    startallnlri_r(vm->bgp);

    const netaddr_t *addr;
    while ((addr = nextnlri_r(vm->bgp)) != NULL)
        vm_pushaddr(vm, addr);

    if (unlikely(endnlri_r(vm->bgp) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

inline void vm_exec_nlri_insert(filter_vm_t *vm)
{
    if (unlikely(getbgptype_r(vm->bgp) != BGP_UPDATE))
        vm_abort(vm, VM_PACKET_MISMATCH);

    startnlri_r(vm->bgp);

    const netaddr_t *addr;
    while ((addr = nextnlri_r(vm->bgp)) != NULL) {
        patricia_trie_t *trie = vm->curtrie;
        switch (addr->family) {
        case AF_INET6:
            trie = vm->curtrie6;
            // fallthrough
        case AF_INET:
            if (!patinsertn(trie, addr, NULL))
                vm_abort(vm, VM_OUT_OF_MEMORY);

            break;
        default:
            vm_abort(vm, VM_SURPRISING_BYTES); // should never happen
            break;
        }
    }

    if (unlikely(endnlri_r(vm->bgp) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

inline void vm_exec_nlri_accumulate(filter_vm_t *vm)
{
    if (unlikely(getbgptype_r(vm->bgp) != BGP_UPDATE))
        vm_abort(vm, VM_PACKET_MISMATCH);

    startnlri_r(vm->bgp);

    const netaddr_t *addr;
    while ((addr = nextnlri_r(vm->bgp)) != NULL)
        vm_pushaddr(vm, addr);

    if (unlikely(endnlri_r(vm->bgp) != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);
}

void vm_exec_exact(filter_vm_t *vm, int access);
void vm_exec_subnet(filter_vm_t *vm, int access);
void vm_exec_supernet(filter_vm_t *vm, int access);
void vm_exec_related(filter_vm_t *vm, int access);

inline void vm_exec_pfxcontains(filter_vm_t *vm, int kidx)
{
    if (unlikely((unsigned int) kidx >= vm->ksiz))
        vm_abort(vm, VM_K_UNDEFINED);

    stack_cell_t *a = &vm->kp[kidx];
    while (vm->si > 0) {
        stack_cell_t *b = vm_pop(vm);

        if (prefixeq(&a->addr, &b->addr)) {
            vm_clearstack(vm);
            vm_pushvalue(vm, true);
            return;
        }
    }

    vm_pushvalue(vm, false);
}

inline void vm_exec_addrcontains(filter_vm_t *vm, int kidx)
{
    if (unlikely((unsigned int) kidx >= vm->ksiz))
        vm_abort(vm, VM_K_UNDEFINED);

    stack_cell_t *a = &vm->kp[kidx];
    while (vm->si > 0) {
        stack_cell_t *b = vm_pop(vm);

        if (naddreq(&a->addr, &b->addr)) {
            vm_clearstack(vm);
            vm_pushvalue(vm, true);
            return;
        }
    }

    vm_pushvalue(vm, false);
}

inline void vm_exec_ascontains(filter_vm_t *vm, int kidx)
{
    if (unlikely((unsigned int) kidx >= vm->ksiz))
        vm_abort(vm, VM_K_UNDEFINED);

    stack_cell_t *a = &vm->kp[kidx];
    while (vm->si > 0) {
        stack_cell_t *b = vm_pop(vm);
        if (a->as == b->as) {
            vm_clearstack(vm);
            vm_pushvalue(vm, true);
            return;
        }
    }

    vm_pushvalue(vm, false);
}

void vm_exec_aspmatch(filter_vm_t *vm, int access);
void vm_exec_aspstarts(filter_vm_t *vm, int access);
void vm_exec_aspends(filter_vm_t *vm, int access);
void vm_exec_aspexact(filter_vm_t *vm, int access);

#endif

