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
 * @file isolario/vt100.h
 *
 * @brief ANSI VT100 compliant console escape codes.
 */

#ifndef ISOLARIO_VT100_H_
#define ISOLARIO_VT100_H_

/**
 * @defgroup VT100 Macros for ANSI VT100 consoles
 *
 * @brief Escape sequences that trigger various advanced VT100 console features.
 *
 * @{
 */

#define VTVLN "\x1b(0\x78\x1b(B"  ///< ANSI VT100 vertical line
#define VTBLC "\x1b(0\x6d\x1b(B"  ///< ANSI VT100 lower left corner
#define VTHLN "\x1b(0\x71\x1b(B"  ///< ANSI VT100 horizontal line
#define VTBLD "\x1b[1m"           ///< ANSI VT100 bold
#define VTLIN "\x1b[2m"           ///< ANSI VT100 low intensity
#define VTITL "\x1b[3m"           ///< ANSI VT100 italics
#define VTRST "\x1b[0m"           ///< ANSI VT100 reset
#define VTRED "\x1b[41m"          ///< ANSI VT100 background to red
#define VTGRN "\x1b[32m"          ///< ANSI VT100 background to green
#define VTYLW "\x1b[33m"          ///< ANSI VT100 background to yellow
#define VTBLU "\x1b[34m"          ///< ANSI VT100 background to blue
#define VTMGN "\x1b[35m"          ///< ANSI VT100 background to magenta
#define VTCYN "\x1b[36m"          ///< ANSI VT100 background to cyan
#define VTWHT "\x1b[37m"          ///< ANSI VT100 background to white

/** @} */

#endif
