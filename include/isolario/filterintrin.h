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

void vm_growstack(filter_vm_t *vm);

inline void vm_clearstack(filter_vm_t *vm)
{
    vm->si = 0;
}

inline noreturn void vm_abort(filter_vm_t *vm, int error)
{
    vm->error = error;
    longjmp(vm->except, -1);
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

#endif

