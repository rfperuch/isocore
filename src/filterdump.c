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
#include <isolario/vt100.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum {
    ARG_NONE,
    ARG_DIRECT,
    ARG_K,
    ARG_FN,
    ARG_TRIE,
    ARG_ACC_NETS,  // NLRI/WITHDRAWN
    ARG_ACC_PATH   // AS/AS4/REAL AS path
};

static const char *const vm_opstr_table[256] = {
    [FOPC_NOP]          = "NOP",
    [FOPC_BLK]          = "BLK",
    [FOPC_ENDBLK]       = "ENDBLK",
    [FOPC_LOAD]         = "LOAD",
    [FOPC_LOADK]        = "LOADK",
    [FOPC_UNPACK]       = "UNPACK",
    [FOPC_EXARG]        = "EXARG",
    [FOPC_STORE]        = "STORE",
    [FOPC_DISCARD]      = "DISCARD",
    [FOPC_NOT]          = "NOT",
    [FOPC_CPASS]        = "CPASS",
    [FOPC_CFAIL]        = "CFAIL",
    [FOPC_SETTLE]       = "SETTLE",
    [FOPC_EXACT]        = "EXACT",
    [FOPC_SUBNET]       = "SUBNET",
    [FOPC_SUPERNET]     = "SUPERNET",
    [FOPC_RELATED]      = "RELATED",
    [FOPC_PFXCONTAINS]  = "PFXCONTAINS",
    [FOPC_ADDRCONTAINS] = "ADDRCONTAINS",
    [FOPC_ASCONTAINS]   = "ASCONTAINS",
    [FOPC_CALL]         = "CALL",
    [FOPC_ASPMATCH]     = "ASPMATCH",
    [FOPC_ASPSTARTS]    = "ASPSTARTS",
    [FOPC_ASPENDS]      = "ASPENDS",
    [FOPC_ASPEXACT]     = "ASPEXACT",
    [FOPC_SETTRIE]      = "SETTRIE",
    [FOPC_SETTRIE6]     = "SETTRIE6",
    [FOPC_CLRTRIE]      = "CLRTRIE",
    [FOPC_CLRTRIE6]     = "CLRTRIE6",
    [FOPC_ASCMP]        = "ASCMP",
    [FOPC_ADDRCMP]      = "ADDRCMP",
    [FOPC_PFXCMP]       = "PFXCMP"
};

static const int8_t vm_oparg_table[OPCODES_COUNT] = {
    [FOPC_NOP]          = ARG_NONE,
    [FOPC_BLK]          = ARG_NONE,
    [FOPC_ENDBLK]       = ARG_NONE,
    [FOPC_LOAD]         = ARG_DIRECT,
    [FOPC_LOADK]        = ARG_K,
    [FOPC_UNPACK]       = ARG_NONE,
    [FOPC_EXARG]        = ARG_DIRECT,
    [FOPC_STORE]        = ARG_NONE,
    [FOPC_DISCARD]      = ARG_NONE,
    [FOPC_NOT]          = ARG_NONE,
    [FOPC_CPASS]        = ARG_NONE,
    [FOPC_CFAIL]        = ARG_NONE,
    [FOPC_SETTLE]       = ARG_NONE,
    [FOPC_EXACT]        = ARG_ACC_NETS,
    [FOPC_SUBNET]       = ARG_ACC_NETS,
    [FOPC_SUPERNET]     = ARG_ACC_NETS,
    [FOPC_RELATED]      = ARG_ACC_NETS,
    [FOPC_PFXCONTAINS]  = ARG_K,
    [FOPC_ADDRCONTAINS] = ARG_K,
    [FOPC_ASCONTAINS]   = ARG_K,
    [FOPC_ASPMATCH]     = ARG_ACC_PATH,
    [FOPC_ASPSTARTS]    = ARG_ACC_PATH,
    [FOPC_ASPENDS]      = ARG_ACC_PATH,
    [FOPC_ASPEXACT]     = ARG_ACC_PATH,
    [FOPC_CALL]         = ARG_FN,
    [FOPC_SETTRIE]      = ARG_TRIE,
    [FOPC_SETTRIE6]     = ARG_TRIE,
    [FOPC_CLRTRIE]      = ARG_NONE,
    [FOPC_CLRTRIE6]     = ARG_NONE,
    [FOPC_ASCMP]        = ARG_K,
    [FOPC_ADDRCMP]      = ARG_K,
    [FOPC_PFXCMP]       = ARG_K
};

#define BADOPCOL  VTREDB VTWHT
#define HEXCOL    VTLIN
#define OPNAMECOL VTBLD
#define ERRCOL    VTRED
#define WARNCOL   VTYLW
#define COMMCOL   VTITL

enum { COMM_INFO, COMM_WARN, COMM_ERR };

static void comment(FILE *f, int mode, int colors, const char *msg, ...)
{
    if (colors)
        fputs(COMMCOL, f);

    fputs("; ", f);
    if (mode == COMM_WARN)
        fputs(WARNCOL, f);
    else if (mode == COMM_ERR)
        fputs(ERRCOL, f);

    va_list va;
    va_start(va, msg);
    vfprintf(f, msg, va);
    va_end(va);

    if (colors)
        fputs(VTRST, f);
}

static void explain_function(FILE *f, int colors, int fn)
{
    const char *name = NULL;
    switch (fn) {
    case VM_WITHDRAWN_INSERT_FN:
    case VM_WITHDRAWN_ACCUMULATE_FN:
        name = "packet.withdrawn";
        break;
    case VM_ALL_WITHDRAWN_INSERT_FN:
    case VM_ALL_WITHDRAWN_ACCUMULATE_FN:
        name = "packet.every_withdrawn";
        break;
    case VM_NLRI_INSERT_FN:
    case VM_NLRI_ACCUMULATE_FN:
        name = "packet.nlri";
        break;
    case VM_ALL_NLRI_INSERT_FN:
    case VM_ALL_NLRI_ACCUMULATE_FN:
        name = "packet.every_nlri";
        break;
    default:
        break;
    }

    if (name)
        comment(f, COMM_INFO, colors, "calls: %s", name);
}

static void explain_access(FILE *f, int colors, int access_type, int mask)
{
    char buf[256];

    buf[0] = '\0';
    if (mask & FOPC_ACCESS_SETTLE) {
        strcat(buf, "SETTLE+");
        mask &= ~FOPC_ACCESS_SETTLE;
    }
    switch (access_type) {
    case ARG_ACC_NETS:
        if (mask & FOPC_ACCESS_ALL) {
            strcat(buf, "ALL_");
            mask &= ~FOPC_ACCESS_ALL;
        }
        if (bitpopcnt(mask) != 1)
            break; // bad access flags (must have exactly one between NLRI and WITHDRAWN)

        if (mask & FOPC_ACCESS_NLRI) {
            strcat(buf, "NLRI");
            mask &= ~FOPC_ACCESS_NLRI;
        }
        if (mask & FOPC_ACCESS_WITHDRAWN) {
            strcat(buf, "WITHDRAWN");
            mask &= ~FOPC_ACCESS_WITHDRAWN;
        }
        break;
    case ARG_ACC_PATH:
        if (bitpopcnt(mask) != 1)
            break; // bad access flags (must have exactly one between AS/AS4 and REAL AS)
        if (mask & FOPC_ACCESS_AS_PATH) {
            strcat(buf, "AS_PATH");
            mask &= ~FOPC_ACCESS_AS_PATH;
        }
        if (mask & FOPC_ACCESS_AS4_PATH) {
            strcat(buf, "AS4_PATH");
            mask &= ~FOPC_ACCESS_AS4_PATH;
        }
        if (mask & FOPC_ACCESS_REAL_AS_PATH) {
            strcat(buf, "REAL_AS_PATH");
            mask &= ~FOPC_ACCESS_REAL_AS_PATH;
        }
        break;
    default:
        assert(false);
        break;
    }

    // if mask was empty or it contained errors, print error
    if (buf[0] != '\0' && mask == 0)
        comment(f, COMM_INFO, colors, "%s", buf);
    else
        comment(f, COMM_ERR, colors, "<BAD_ACCESS:%#x>", (unsigned int) mask);
}

static void explain_block(FILE *f, int colors, int pc, int codesiz, int blksize)
{
    if (pc + blksize >= codesiz)
        comment(f, COMM_ERR, colors, "block jumps over the end of code!");
    else
        comment(f, COMM_INFO, colors, "to line: %d", pc + blksize + 1);
}

static void prolog(FILE *f, int pc, bytecode_t code, int colors)
{
    fprintf(f, "%5d: ", pc + 1);
    if (colors)
        fputs(HEXCOL, f);

    fprintf(f, "%#.4x", (int) code);
    if (colors)
        fputs(VTRST, f);
}

static void printbad(FILE *f, bytecode_t code, int colors)
{
    if (colors)
        fputs(BADOPCOL, f);

    fprintf(f, "<ILLEGAL:%#x>", (unsigned int) code);
    if (colors)
        fputs(VTRST, f);
}

static int printop(FILE *f, int pc, int codesiz, bytecode_t code, const char *name, int exarg, int colors)
{
    if (colors)
        fputs(OPNAMECOL, f);

    fputs(name, f);
    if (colors)
        fputs(VTRST, f);

    int opcode = vm_getopcode(code);
    int arg    = vm_getarg(code);
    int mode   = vm_oparg_table[opcode];
    if (mode == ARG_NONE) {
        if (arg != 0) {
            fputs("\t\t", f);
            comment(f, COMM_WARN, colors, "spurious opcode argument: %d", arg);
        }

        return false;
    }

    fputc('\t', f);
    arg = vm_extendarg(arg, exarg);
    switch (mode) {
    case ARG_DIRECT:
        fprintf(f, "%d", arg);
        break;

    case ARG_K:
        fprintf(f, "K[%d]", arg);
        break;

    case ARG_FN:
        fprintf(f, "Fn[%d]", arg);
        break;

    case ARG_TRIE:
        fprintf(f, "Tr[%d]", arg);
        break;

    case ARG_ACC_NETS:
    case ARG_ACC_PATH:
        fprintf(f, "Ac[%#x]", (unsigned int) arg);
        break;

    default:
        assert(false);
        return false;
    }

    if (exarg != 0) {
        fputc('\t', f);
        comment(f, COMM_INFO, colors, "original argument: %d extended", arg);
    }
    if (opcode == FOPC_CALL) {
        fputc('\t', f);
        explain_function(f, colors, arg);
    }
    if (mode == ARG_ACC_NETS || mode == ARG_ACC_PATH) {
        fputc('\t', f);
        explain_access(f, colors, mode, arg);
    }
    if (opcode == FOPC_BLK) {
        fputc('\t', f);
        explain_block(f, colors, pc, codesiz, arg);
    }
    return true;
}

void filter_dump(FILE *f, filter_vm_t *vm)
{
    int colors = isvt100tty(fileno(f));
    int exarg = 0;

    for (int pc = 0; pc < vm->codesiz; pc++) {
        bytecode_t ip = vm->code[pc];

        const char *name = vm_opstr_table[vm_getopcode(ip)];

        prolog(f, pc, ip, colors);
        fputc(' ', f);

        int consumed = false;
        if (name)
            consumed = printop(f, pc, vm->codesiz, ip, name, exarg, colors);
        else
            printbad(f, ip, colors);

        fputc('\n', f);

        if (ip == FOPC_EXARG) {
            exarg <<= 8;
            exarg |= vm_getarg(ip);
        }
        if (consumed)
            exarg = 0;
    }
}

