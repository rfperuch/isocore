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

// =====================================================================
// NOTE: THIS FILE HAS NO INCLUDE GUARDS, BECAUSE IT NEEDS TO COPY-PASTE
//       A STATIC OPCODE TABLE FOR A COMPUTED GOTO STATEMENT
//
// KEEP THIS TABLE IN SYNC WITH isolario/filterpacket.h
// =====================================================================

static const void *const vm_opcode_table[256] = {
    // clear array to sigill, for opcodes go below...

    // opcode table:
    [FOPC_NOP]          = &&EX_NOP,
    [FOPC_BLK]          = &&EX_BLK,
    [FOPC_LOAD]         = &&EX_LOAD,
    [FOPC_LOADK]        = &&EX_LOADK,
    [FOPC_UNPACK]       = &&EX_UNPACK,
    [FOPC_EXARG]        = &&EX_EXARG,
    [FOPC_STORE]        = &&EX_STORE,
    [FOPC_DISCARD]      = &&EX_DISCARD,
    [FOPC_NOT]          = &&EX_NOT,
    [FOPC_CPASS]        = &&EX_CPASS,
    [FOPC_CFAIL]        = &&EX_CFAIL,

    [FOPC_EXACT]        = &&EX_EXACT,
    [FOPC_SUBNET]       = &&EX_SUBNET,
    [FOPC_PSUBNET]      = &&EX_PSUBNET,
    [FOPC_SUPERNET]     = &&EX_SUPERNET,
    [FOPC_RELATED]      = &&EX_RELATED,
    [FOPC_PFXCONTAINS]  = &&EX_PFXCONTAINS,
    [FOPC_ADDRCONTAINS] = &&EX_ADDRCONTAINS,
    [FOPC_ASCONTAINS]   = &&EX_ASCONTAINS,

    [FOPC_ASPMATCH]     = &&EX_ASPMATCH,
    [FOPC_ASPSTARTS]    = &&EX_ASPSTARTS,
    [FOPC_ASPENDS]      = &&EX_ASPENDS,
    [FOPC_ASPEXACT]     = &&EX_ASPEXACT,

    [FOPC_CALL]         = &&EX_CALL,
    [FOPC_SETTRIE]      = &&EX_SETTRIE,
    [FOPC_SETTRIE6]     = &&EX_SETTRIE6,
    [FOPC_CLRTRIE]      = &&EX_CLRTRIE,
    [FOPC_CLRTRIE6]     = &&EX_CLRTRIE6,
    [FOPC_ASCMP]        = &&EX_ASCMP,
    [FOPC_ADDRCMP]      = &&EX_ADDRCMP,
    [FOPC_PFXCMP]       = &&EX_PFXCMP,

    [OPCODES_COUNT]     = &&EX_SIGILL,

    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL, &&EX_SIGILL,
    &&EX_SIGILL
};

