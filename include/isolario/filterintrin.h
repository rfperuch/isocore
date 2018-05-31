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

#ifndef ISOLARIO_FILTERINTRIN_H_
#define ISOLARIO_FILTERINTRIN_H_

#include <assert.h>
#include <isolario/netaddr.h>
#include <isolario/filterpacket.h>
#include <setjmp.h>
#include <stdnoreturn.h>
#include <string.h>

enum {
    BAD_OPCODE = -1,

    FOPC_NOP,      ///< NOP: does nothing
    FOPC_BLK,      ///< NONE: push a new block in expressions stack.
    FOPC_LOAD,     ///< PUSH: direct value load
    FOPC_LOADK,    ///< PUSH: load from constants environment
    FOPC_EXARG,    ///< NOP: extends previous operation's argument range
    FOPC_STORE,    ///< POP: store address into current trie (v4 or v6 depends on address)
    FOPC_DISCARD,  ///< POP: remove address from current trie (v4 or v6 depends on address)
    FOPC_NOT,      ///< POP-PUSH: pops stack topmost value and negates it
    FOPC_CPASS,    ///< POP: pops topmost stack element and terminates with PASS if value is true.
    FOPC_CFAIL,    ///< POP: pops topmost stack element and terminates with FAIL if value is false.
    FOPC_EXACT,    ///< POPA-PUSH: pops the entire stack and verifies that all addresses are exactly in the tries, pushes the result.
    FOPC_ANY,      ///< POPA-PUSH: pops the entire stack and verifies that at least one address is exactly in the tries, pushes the result.
    FOPC_SUBNET,
    FOPC_CALL,     ///< ???: call a function
    FOPC_SETTRIE,
    FOPC_SETTRIE6,
    FOPC_CLRTRIE,
    FOPC_CLRTRIE6,

    OPCODES_COUNT
};

void vm_growstack(filter_vm_t *vm);

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

inline void vm_clearstack(filter_vm_t *vm)
{
    vm->si = 0;
}

inline noreturn void vm_abort(filter_vm_t *vm, int error)
{
    vm->error = error;
    longjmp(vm->except, -1);
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

inline void vm_exec_loadk(filter_vm_t *vm, int kidx)
{
    if (unlikely(kidx >= vm->ksiz))
        vm_abort(vm, VM_K_UNDEFINED);

    vm_push(vm, &vm->kp[kidx]);
}

inline void vm_exec_store(filter_vm_t *vm)
{
    stack_cell_t *cell = vm_pop(vm);
    netaddr_t *addr    = &cell->addr;
    switch (addr->family) {
    case AF_INET:
        if (!patinsertn(vm->curtrie, addr, NULL))
            vm_abort(vm, VM_OUT_OF_MEMORY);

        break;
    case AF_INET6:
        if (!patinsertn(vm->curtrie6, addr, NULL))
            vm_abort(vm, VM_OUT_OF_MEMORY);

        break;
    default:
        vm_abort(vm, VM_SURPRISING_BYTES); // should never happen
        break;
    }
}

inline void vm_exec_discard(filter_vm_t *vm)
{
    stack_cell_t *cell = vm_pop(vm);
    netaddr_t *addr    = &cell->addr;
    switch (addr->family) {
    case AF_INET:
        patremoven(vm->curtrie, addr);
        break;
    case AF_INET6:
        patremoven(vm->curtrie6, addr);
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
    if (unlikely(trie >= vm->ntries))
        vm_abort(vm, VM_TRIE_UNDEFINED);

    vm->curtrie = &vm->tries[trie];
    if (unlikely(vm->curtrie->family != AF_INET))
        vm_abort(vm, VM_TRIE_MISMATCH);
}

inline void vm_exec_settrie6(filter_vm_t *vm, int trie6)
{
    if (unlikely(trie6 >= vm->ntries))
        vm_abort(vm, VM_TRIE_UNDEFINED);

    vm->curtrie6 = &vm->tries[trie6];
    if (unlikely(vm->curtrie6->family != AF_INET6))
        vm_abort(vm, VM_TRIE_MISMATCH);
}

void vm_exec_withdrawn_accumulate(filter_vm_t *vm);
void vm_exec_withdrawn_insert(filter_vm_t *vm);

void vm_exec_nlri_accumulate(filter_vm_t *vm);
void vm_exec_nlri_insert(filter_vm_t *vm);

void vm_exec_exact(filter_vm_t *vm);

#endif
