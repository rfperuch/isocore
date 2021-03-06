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

extern void vm_pushas(filter_vm_t *vm, wide_as_t as);

extern void vm_exec_loadk(filter_vm_t *vm, int kidx);

extern void vm_exec_break(filter_vm_t *vm);

extern void vm_check_array(filter_vm_t *vm, stack_cell_t *arr);

extern void *vm_heap_ptr(filter_vm_t *vm, intptr_t off);

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

extern void vm_exec_settle(filter_vm_t *vm);

void vm_exec_hasattr(filter_vm_t *vm, int code)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    vm_exec_settle(vm);

    // optimize notable attributes
    bgpattr_t *ptr = NULL;
    switch (code) {
    case ORIGIN_CODE:
        ptr = getbgporigin_r(vm->bgp);
        break;
    case NEXT_HOP_CODE:
        ptr = getbgpnexthop_r(vm->bgp);
        break;
    case AGGREGATOR_CODE:
        ptr = getbgpaggregator_r(vm->bgp);
        break;
    case AS4_AGGREGATOR_CODE:
        ptr = getbgpas4aggregator_r(vm->bgp);
        break;
    case ATOMIC_AGGREGATE_CODE:
        ptr = getbgpatomicaggregate_r(vm->bgp);
        break;
    case AS_PATH_CODE:
        ptr = getbgpaspath_r(vm->bgp);
        break;
    case AS4_PATH_CODE:
        ptr = getbgpas4path_r(vm->bgp);
        break;
    case MP_REACH_NLRI_CODE:
        ptr = getbgpmpreach_r(vm->bgp);
        break;
    case MP_UNREACH_NLRI_CODE:
        ptr = getbgpmpunreach_r(vm->bgp);
        break;
    case COMMUNITY_CODE:
        ptr = getbgpcommunities_r(vm->bgp);
        break;
    case EXTENDED_COMMUNITY_CODE:
        ptr = getbgpexcommunities_r(vm->bgp);
        break;
    case LARGE_COMMUNITY_CODE:
        ptr = getbgplargecommunities_r(vm->bgp);
        break;
    default:
        // no luck, plain iteration
        startbgpattribs_r(vm->bgp);
        while ((ptr = nextbgpattrib_r(vm->bgp)) != NULL) {
            if (ptr->code == code)
                break;
        }
        endbgpattribs_r(vm->bgp);
    }

    assert(ptr == NULL || ptr->code == code);

    vm_pushvalue(vm, ptr != NULL);
}

extern void vm_exec_all_withdrawn_insert(filter_vm_t *vm);
extern void vm_exec_all_withdrawn_accumulate(filter_vm_t *vm);
extern void vm_exec_withdrawn_accumulate(filter_vm_t *vm);
extern void vm_exec_withdrawn_insert(filter_vm_t *vm);

extern void vm_exec_all_nlri_insert(filter_vm_t *vm);
extern void vm_exec_all_nlri_accumulate(filter_vm_t *vm);
extern void vm_exec_nlri_insert(filter_vm_t *vm);
extern void vm_exec_nlri_accumulate(filter_vm_t *vm);

extern void vm_exec_settle(filter_vm_t *vm);

void vm_prepare_addr_access(filter_vm_t *vm, unsigned short mode)
{
    if (mode & FOPC_ACCESS_SETTLE)
        vm_exec_settle(vm);
    if (vm->access_mask == mode)
        return;

    switch (mode & ~FOPC_ACCESS_SETTLE) {
    case FOPC_ACCESS_WITHDRAWN | FOPC_ACCESS_ALL:
        startallwithdrawn_r(vm->bgp);
        vm->settle_func = endwithdrawn_r;
        break;
    case FOPC_ACCESS_WITHDRAWN:
        startwithdrawn_r(vm->bgp);
        vm->settle_func = endwithdrawn_r;
        break;
    case FOPC_ACCESS_NLRI | FOPC_ACCESS_ALL:
        startallnlri_r(vm->bgp);
        vm->settle_func = endnlri_r;
        break;
    case FOPC_ACCESS_NLRI:
        startnlri_r(vm->bgp);
        vm->settle_func = endnlri_r;
        break;
    default:
        // shouldn't be possible...
        vm_abort(vm, VM_BAD_ACCESSOR);
        break;
    }

    vm->access_mask = mode;
}

void vm_prepare_as_access(filter_vm_t *vm, unsigned short mode)
{
    if (mode & FOPC_ACCESS_SETTLE)
        vm_exec_settle(vm);
    if (vm->access_mask == mode)
        return;

    switch (mode & ~FOPC_ACCESS_SETTLE) {
    case FOPC_ACCESS_AS_PATH:
        startaspath_r(vm->bgp);
        break;
    case FOPC_ACCESS_AS4_PATH:
        startas4path_r(vm->bgp);
        break;
    case FOPC_ACCESS_REAL_AS_PATH:
        startrealaspath_r(vm->bgp);
        break;
    default:
        // shouldn't be possible...
        vm_abort(vm, VM_BAD_ACCESSOR);
        break;
    }

    vm->settle_func = endaspath_r;
    vm->access_mask = mode;
}

void vm_exec_exact(filter_vm_t *vm, int access)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    vm_prepare_addr_access(vm, access);

    int result = false;
    while (true) {
        netaddr_t *addr = (access & FOPC_ACCESS_NLRI) ?
                          nextnlri_r(vm->bgp) :
                          nextwithdrawn_r(vm->bgp);
        if (!addr)
            break;

        patricia_trie_t *trie = vm->curtrie;
        switch (addr->family) {
        case AF_INET6:
            trie = vm->curtrie6;
            // fallthrough
        case AF_INET:
            if (patsearchexactn(trie, addr)) {
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
    vm_pushvalue(vm, result);
}

void vm_exec_subnet(filter_vm_t *vm, int access)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    vm_prepare_addr_access(vm, access);

    int result = false;
    while (true) {
        netaddr_t *addr = (access & FOPC_ACCESS_NLRI) ?
                          nextnlri_r(vm->bgp) :
                          nextwithdrawn_r(vm->bgp);
        if (!addr)
            break;

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
    vm_pushvalue(vm, result);
}

void vm_exec_supernet(filter_vm_t *vm, int access)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    vm_prepare_addr_access(vm, access);

    int result = false;
    while (true) {
        netaddr_t *addr = (access & FOPC_ACCESS_NLRI) ?
                    nextnlri_r(vm->bgp) :
                    nextwithdrawn_r(vm->bgp);
        if (!addr)
            break;

        patricia_trie_t *trie = vm->curtrie;
        switch (addr->family) {
        case AF_INET6:
            trie = vm->curtrie6;
            // fallthrough
        case AF_INET:
            if (patissupernetofn(trie, addr)) {
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
    vm_pushvalue(vm, result);
}


void vm_exec_related(filter_vm_t *vm, int access)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    vm_prepare_addr_access(vm, access);

    int result = false;
    while (true) {
        netaddr_t *addr = (access & FOPC_ACCESS_NLRI) ?
                    nextnlri_r(vm->bgp) :
                    nextwithdrawn_r(vm->bgp);
        if (!addr)
            break;

        patricia_trie_t *trie = vm->curtrie;
        switch (addr->family) {
        case AF_INET6:
            trie = vm->curtrie6;
            // fallthrough
        case AF_INET:
            if (patisrelatedofn(trie, addr)) {
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
    vm_pushvalue(vm, result);
}

extern void vm_exec_pfxcontains(filter_vm_t *vm, int kidx);

extern void vm_exec_addrcontains(filter_vm_t *vm, int kidx);

extern void vm_exec_ascontains(filter_vm_t *vm, int kidx);

void vm_exec_aspmatch(filter_vm_t *vm, int access)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    vm_prepare_as_access(vm, access);

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
                    return;
                }

                asbuf[buffered++] = ent->as;
            }
            if (cell->as != asbuf[i] && cell->as != AS_ANY)
                break;
        }

        if (i == vm->si) {
            // successfuly matched
            vm_clearstack(vm);
            vm->sp[vm->si++].value = true;
            return;
        }

        // didn't match, consume the first element in asbuf and continue trying
        memmove(asbuf, asbuf + 1, (buffered - 1) * sizeof(*asbuf));
        buffered--;
    }
}

void vm_exec_aspstarts(filter_vm_t *vm, int access)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    vm_prepare_as_access(vm, access);

    int i;
    for (i = 0; i < vm->si; i++) {
        as_pathent_t *ent = nextaspath_r(vm->bgp);
        if (!ent)
            break;
        if (vm->sp[i].as != ent->as && vm->sp[i].as != AS_ANY)
            break;  // does not match
    }

    int value = (i == vm->si);

    vm_clearstack(vm);
    vm->sp[vm->si++].value = value;
}

void vm_exec_aspends(filter_vm_t *vm, int access)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    vm_prepare_as_access(vm, access);

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

    int length_match = (n == vm->si);

    vm_clearstack(vm);

    if (!length_match) {
        vm->sp[vm->si++].value = false;
        return;
    }

    // NOTE: technically we cleared the stack, but we know it was N elements long before...
    int i;
    for (i = 0; i < n; i++) {
        if (asbuf[i] != vm->sp[i].as && vm->sp[i].as != AS_ANY)
            break;
    }

    vm->sp[vm->si++].value = (i == n);
}

void vm_exec_aspexact(filter_vm_t *vm, int access)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    vm_prepare_as_access(vm, access);

    as_pathent_t *ent;
    int i;
    for (i = 0; i < vm->si; i++) {
        ent = nextaspath_r(vm->bgp);
        if (!ent)
            break;
        if (vm->sp[i].as != ent->as && vm->sp[i].as != AS_ANY)
            break;  // does not match
    }

    ent = nextaspath_r(vm->bgp);

    int value = (ent == NULL && i == vm->si);

    vm_clearstack(vm);
    vm->sp[vm->si++].value = value;
}

void vm_exec_commexact(filter_vm_t *vm)
{
    if (getbgptype_r(vm->bgp) != BGP_UPDATE)
        vm_abort(vm, VM_PACKET_MISMATCH);

    startcommunities_r(vm->bgp, COMMUNITY_CODE);

    bool seen[vm->si];
    int seen_count = 0;

    memset(seen, 0, vm->si * sizeof(*seen));

    community_t *comm;
    while ((comm = nextcommunity_r(vm->bgp)) != NULL && seen_count != vm->si) {
        for (int i = 0; i < vm->si; i++) {
            if (*comm == vm->sp[i].comm) {
                if (!seen[i]) {
                    seen[i] = true;
                    seen_count++;
                }

                break;
            }
        }
    }

    endcommunities_r(vm->bgp);

    int value = (seen_count == vm->si);

    vm_clearstack(vm);
    vm->sp[vm->si++].value = value;
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

intptr_t vm_heap_alloc(filter_vm_t *vm, size_t size, vm_heap_zone_t zone)
{
    // align allocation
    size += sizeof(max_align_t) - 1;
    size -= (size & (sizeof(max_align_t) - 1));

    intptr_t ptr;
    if (unlikely(!vm_heap_ensure(vm, size)))
        return VM_BAD_HEAP_PTR;

    switch (zone) {
    case VM_HEAP_PERM:
        if (unlikely(vm->dynmarker > 0)) {
            assert(false);
            return VM_BAD_HEAP_PTR; // illegal!
        }

        ptr = vm->highwater;
        vm->highwater += size;
        return ptr;

    case VM_HEAP_TEMP:
        ptr = vm->highwater;
        ptr += vm->dynmarker;

        vm->dynmarker += size;
        return ptr;

    default:
        // should never happen
        assert(false);
        return VM_BAD_HEAP_PTR;
    }
}

extern void vm_heap_return(filter_vm_t *vm, size_t size);

intptr_t vm_heap_grow(filter_vm_t *vm, intptr_t addr, size_t newsize)
{
    newsize += sizeof(max_align_t) - 1;
    newsize -= (newsize & (sizeof(max_align_t) - 1));

    if (addr == 0)
        addr = vm->highwater; // never did a VM_HEAP_TEMP alloc before

    size_t oldsize = vm->highwater + vm->dynmarker - addr;
    if (unlikely(newsize < oldsize))
        return addr;

    size_t amount = newsize - oldsize;
    if (unlikely(!vm_heap_ensure(vm, amount)))
        return VM_BAD_HEAP_PTR;

    vm->dynmarker += amount;
    return addr;
}
