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

    FOPC_BLK,      ///< NONE: push a new block in expressions stack.
    FOPC_LOAD,     ///< PUSH: direct value load
    FOPC_LOADK,    ///< PUSH: load from constants environment
    FOPC_UNPACK,   ///< POP-PUSH: unpack an array constant into stack.
    FOPC_EXARG,    ///< NOP: extends previous operation's argument range
    FOPC_STORE,    ///< POP: store address into current trie (v4 or v6 depends on address)
    FOPC_DISCARD,  ///< POP: remove address from current trie (v4 or v6 depends on address)
    FOPC_NOT,      ///< POP-PUSH: pops stack topmost value, negates it and pushes it back.
    FOPC_CPASS,    ///< POP: pops topmost stack element and terminates with PASS if value is true.
    FOPC_CFAIL,    ///< POP: pops topmost stack element and terminates with FAIL if value is false.

    FOPC_EXACT,
        /**<
         * Pops the entire stack and verifies that at least one *address* has an *exact* relationship with
         * the addresses stored inside the current tries, pushes a boolean result.
         *
         * This opcode expects that the entire stack is composed of cells containing \a netaddr_t.
         *
         * @note Stack operation mode is POPA-PUSH, this opcode has no arguments.
         */

    FOPC_SUBNET,
    FOPC_PSUBNET,
    FOPC_SUPERNET,
    FOPC_RELATED,

    FOPC_PFXCONTAINS,    ///< POPA-PUSH: Prefix Contained
    FOPC_ADDRCONTAINS,
    FOPC_ASCONTAINS,

    FOPC_ASPMATCH,
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

enum {
    PKT_ACC_BAD,

    PKT_ACC_WITHDRAWN,
    PKT_ACC_ALL_WITHDRAWN,
    PKT_ACC_NLRI,
    PKT_ACC_ALL_NLRI,
    PKT_ACC_ALL,  // TODO better naming (accesses only NLRI/WITHDRAWN, not ALL)

    PKT_ACC_AS_PATH,
    PKT_ACC_AS4_PATH,
    PKT_ACC_REAL_AS_PATH
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

typedef enum { VM_HEAP_PERM, VM_HEAP_TEMP } vm_heap_zone_t;

enum { VM_BAD_HEAP_PTR = -1 };

intptr_t vm_heap_alloc(filter_vm_t *vm, size_t size, vm_heap_zone_t zone);

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

inline void vm_pushas(filter_vm_t *vm, uint32_t as)
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
        vm_abort(vm, VM_SURPRISING_BYTES); // should never happen
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

inline void vm_exec_exact(filter_vm_t *vm)
{
    while (vm->si > 0) {
        stack_cell_t *cell    = vm_pop(vm);
        netaddr_t *addr       = &cell->addr;
        patricia_trie_t *trie = vm->curtrie;
        switch (addr->family) {
        case AF_INET6:
            trie = vm->curtrie6;
            // fallthrough
        case AF_INET:
            if (patsearchexactn(trie, addr)) {
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

inline void vm_exec_subnet(filter_vm_t *vm)
{
    int result = false;
    while (vm->si > 0) {
        netaddr_t *addr = &vm->sp[--vm->si].addr;

        patricia_trie_t *trie = vm->curtrie;
        switch (addr->family) {
        case AF_INET6:
            trie = vm->curtrie6;
            // fallthrough
        case AF_INET:
            if (patissubnetofn(trie, addr)) {
                result = true;
                goto done;
            }
            break;
        default:
            vm_abort(vm, VM_SURPRISING_BYTES);  // should never happen
            break;
        }
    }

done:
    vm->si = 0;
    vm->sp[vm->si++].value = result;
}

inline void vm_exec_psubnet(filter_vm_t *vm, int access)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    netaddr_t *addr;
    int result, err;

    result = false;
    switch (access) {
    case PKT_ACC_ALL:
        startallnlri_r(vm->bgp);
        while (!result && (addr = nextnlri_r(vm->bgp)) != NULL) {
            patricia_trie_t *trie = vm->curtrie;
            switch (addr->family) {
            case AF_INET6:
                trie = vm->curtrie6;
                // fallthrough
            case AF_INET:
                if (patissubnetofn(trie, addr))
                    result = true;

                break;
            default:
                vm_abort(vm, VM_SURPRISING_BYTES);  // should never happen
                break;
            }
        }

        err = endnlri_r(vm->bgp);
        if (result || err != BGP_ENOERR)
            break;

        startallwithdrawn_r(vm->bgp);
        while (!result && (addr = nextwithdrawn_r(vm->bgp)) != NULL) {
            patricia_trie_t *trie = vm->curtrie;
            switch (addr->family) {
            case AF_INET6:
                trie = vm->curtrie6;
                // fallthrough
            case AF_INET:
                if (patissubnetofn(trie, addr))
                    result = true;

                break;
            default:
                vm_abort(vm, VM_SURPRISING_BYTES);  // should never happen
                break;
            }
        }

        err = endwithdrawn_r(vm->bgp);
        break;
    default:
        vm_abort(vm, VM_BAD_ACCESSOR);
        break;
    }

    if (unlikely(err != BGP_ENOERR))
        vm_abort(vm, VM_BAD_PACKET);

    vm_pushvalue(vm, result);
}

inline void vm_exec_supernet(filter_vm_t *vm)
{
    while (vm->si > 0) {
        stack_cell_t *cell = vm_pop(vm);
        netaddr_t *addr = &cell->addr;
        switch (addr->family) {
        case AF_INET:
            if (patissupernetofn(vm->curtrie, addr)) {
                vm_clearstack(vm);
                vm_pushvalue(vm, true);
                return;
            }
            break;
        case AF_INET6:
            if (patissupernetofn(vm->curtrie6, addr)) {
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


inline void vm_exec_related(filter_vm_t *vm)
{
    while (vm->si > 0) {
        stack_cell_t *cell = vm_pop(vm);
        netaddr_t *addr = &cell->addr;
        switch (addr->family) {
        case AF_INET:
            if (patisrelatedofn(vm->curtrie, addr)) {
                vm_clearstack(vm);
                vm_pushvalue(vm, true);
                return;
            }
            break;
        case AF_INET6:
            if (patisrelatedofn(vm->curtrie6, addr)) {
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

inline void vm_exec_aspmatch(filter_vm_t *vm, int acc)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    switch (acc) {
    case PKT_ACC_AS_PATH:
        startaspath_r(vm->bgp);
        break;
    case PKT_ACC_AS4_PATH:
        startas4path_r(vm->bgp);
        break;
    case PKT_ACC_REAL_AS_PATH:
        startrealaspath_r(vm->bgp);
        break;
    default:
        vm_abort(vm, VM_BAD_ACCESSOR);
        break;
    }

    uint32_t asbuf[vm->si];
    int buffered = 0;

    as_pathent_t *ent;
    while (true) {
        int i;

        for (i = 0; i < vm->si; i++) {
            stack_cell_t *cell = &vm->sp[i];
            if (i == buffered) {
                // read one more AS from packet
                ent = nextaspath_r(vm->bgp);
                if (!ent) {
                    // doesn't match
                    vm_clearstack(vm);
                    vm->sp[vm->si++].value = false;
                    goto done;
                }

                asbuf[buffered++] = ent->as;
            }
            if (cell->as != asbuf[i])
                break;
        }

        if (i == vm->si) {
            // successfuly matched
            vm_clearstack(vm);
            vm->sp[vm->si++].value = true;
            break;
        }

        // didn't match, consume the first element in asbuf and continue trying
        memmove(asbuf, asbuf + 1, (buffered - 1) * sizeof(*asbuf));
        buffered--;
    }

done:
    if (endaspath_r(vm->bgp) != BGP_ENOERR)
        vm_abort(vm, VM_BAD_PACKET);
}

inline void vm_exec_aspstarts(filter_vm_t *vm, int acc)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    switch (acc) {
    case PKT_ACC_AS_PATH:
        startaspath_r(vm->bgp);
        break;
    case PKT_ACC_AS4_PATH:
        startas4path_r(vm->bgp);
        break;
    case PKT_ACC_REAL_AS_PATH:
        startrealaspath_r(vm->bgp);
        break;
    default:
        vm_abort(vm, VM_BAD_ACCESSOR);
        break;
    }

    int i;
    for (i = 0; i < vm->si; i++) {
        as_pathent_t *ent = nextaspath_r(vm->bgp);
        if (!ent || vm->sp[i].as != ent->as)
            break;  // does not match
    }

    if (endaspath_r(vm->bgp) != BGP_ENOERR)
        vm_abort(vm, VM_BAD_PACKET);

    int value = (i == vm->si);

    vm_clearstack(vm);
    vm->sp[vm->si++].value = value;
}


inline void vm_exec_aspends(filter_vm_t *vm, int acc)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    switch (acc) {
    case PKT_ACC_AS_PATH:
        startaspath_r(vm->bgp);
        break;
    case PKT_ACC_AS4_PATH:
        startas4path_r(vm->bgp);
        break;
    case PKT_ACC_REAL_AS_PATH:
        startrealaspath_r(vm->bgp);
        break;
    default:
        vm_abort(vm, VM_BAD_ACCESSOR);
        break;
    }

    uint32_t asbuf[vm->si];
    int n = 0;

    as_pathent_t *ent;
    while ((ent = nextaspath_r(vm->bgp)) != NULL) {
        if (n == vm->si) {
            // slide AS path window
            memmove(asbuf, asbuf + 1, (n - 1) * sizeof(*asbuf));
            n--;
        }

        asbuf[n++] = ent->as;
    }
    if (endaspath_r(vm->bgp) != BGP_ENOERR)
        vm_abort(vm, VM_BAD_PACKET);

    int length_match = (n == vm->si);

    vm_clearstack(vm);

    if (!length_match) {
        vm->sp[vm->si++].value = false;
        return;
    }

    // NOTE: technically we cleared the stack, but we know it was N elements long before...
    int i;
    for (i = 0; i < n; i++) {
        if (asbuf[i] != vm->sp[i].as)
            break;
    }

    vm->sp[vm->si++].value = (i == n);
}


inline void vm_exec_aspexact(filter_vm_t *vm, int acc)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    switch (acc) {
    case PKT_ACC_AS_PATH:
        startaspath_r(vm->bgp);
        break;
    case PKT_ACC_AS4_PATH:
        startas4path_r(vm->bgp);
        break;
    case PKT_ACC_REAL_AS_PATH:
        startrealaspath_r(vm->bgp);
        break;
    default:
        vm_abort(vm, VM_BAD_ACCESSOR);
        break;
    }

    as_pathent_t *ent;
    int i;
    for (i = 0; i < vm->si; i++) {
        ent = nextaspath_r(vm->bgp);
        if (!ent || vm->sp[i].as != ent->as)
            break;  // does not match
    }

    ent = nextaspath_r(vm->bgp);

    int value = (ent == NULL && i == vm->si);

    vm_clearstack(vm);
    vm->sp[vm->si++].value = value;
    if (endaspath_r(vm->bgp) != BGP_ENOERR)
        vm_abort(vm, VM_BAD_PACKET);
}

#endif

