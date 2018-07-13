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
 * @file isolario/branch.h
 *
 * @brief Compiler-specific branch hints.
 */

#ifndef ISOLARIO_BRANCH_H_
#define ISOLARIO_BRANCH_H_

/**
 * @defgroup Branching Compiler-specific branching hints
 *
 * @brief Macros to mark code branches as likely or unlikely.
 *
 * Correct use of this macros give an opportunity to the compiler of optimizing
 * instruction cache usage.
 *
 * @warning Misusing these compiler hints may produce suboptimal code, when in doubt
 *          leave the task to the compiler!
 *
 * @{
 */

/**
 * @def likely(guard)
 * @brief Marks a branch as highly likely.
 *
 * If the compiler honors the hint, generated assembly is optimized for
 * the case in which \a guard is true.
 */
/**
 * @def unlikely(guard)
 * @brief Marks a branch as highly unlikely.
 *
 * If the compiler honors the hint, generated assembly is optimized
 * for the case in which \a guard is false.
 */
/**
 * @def unreachable()
 * @brief Marks a code path as unreachable.
 *
 * The compiler is free to assume that a code path in which this
 * marker appears shall never be reached.
 * 
 * @warning If that code path is actually reachable, then subtle, unexpected and
 *          typically cathastrophic things could very well occur.
 */
#ifdef __GNUC__

#ifndef likely
#define likely(guard) __builtin_expect(!!(guard), 1)
#endif

#ifndef unlikely
#define unlikely(guard) __builtin_expect(!!(guard), 0)
#endif

#ifndef unreachable
#define unreachable() __builtin_unreachable()
#endif

#else

#ifndef likely
#define likely(guard) (guard)
#endif

#ifndef unlikely
#define unlikely(guard) (guard)
#endif

#ifndef unreachable
#define unreachable()
#endif

#endif

/** @} */

#endif
