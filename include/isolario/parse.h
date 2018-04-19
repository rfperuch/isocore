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
 * @file isolario/parse.h
 *
 * @brief Simple whitespace separated token parsing.
 *
 * @note This file is guaranteed to include standard \a stdio.h,
 *       a number of other files may be included in the interest of providing
 *       API functionality, but the includer of this file should not rely on
 *       any other header being included.
 */

#ifndef ISOLARIO_PARSE_H_
#define ISOLARIO_PARSE_H_

#include <stdio.h>

/**
 * @defgroup Parse Simple parser for basic whitespace separated tokens
 *
 * @brief This group provides a basic parser for simple whitespace separated
 *        text formats.
 *
 * The parser has a maximum token length limit, in the interest of simple and
 * rapid parsing with moderated memory usage. The implemented format only
 * supports two basic concepts:
 *
 * * Tokens - A token is any word separated by whitespaces, no quoting
 *            or escape sequences are recognized.
 *
 * * Comments - A hash (\a #) starts a comment that extends to the end of
 *              the line it appears in; \a # is a reserved character and
 *              it may not be used inside a token. A comment may appear
 *              anywhere in text.
 *
 * The parser also recognizes the following escape sequences:
 * * '\n'  - newline
 * * '\v'  - vertical tab
 * * '\t'  - horizontal tab
 * * '\r'  - caret return
 * * '\#'  - hash character
 * * '\\'  - backslash character
 * * '\ '  - space character
 * * '\\n' - token follows after newline
 *
 * Error handling is performed via callbacks, an error callback may be
 * registered to handle parsing errors, when such error is encountered,
 * a callback is invoked with a human readable message, when available,
 * the current parsing session name and line number are also provided.
 * The user is responsible to terminate parsing under such circumstances,
 * typically by using longjmp() or similar methods.
 *
 * @note Since the most effective way to terminate parsing is using longjmp(),
 *       extra care should be taken to avoid using Variable Length Arrays (VLA)
 *       during parsing operation, since doing so may cause memory leaks.
 *
 * @{
 */

enum {
    TOK_LEN_MAX = 64 ///< Maximum token length.
};

/// @brief Error handling callback for parser.
typedef void (*parse_err_callback_t)(const char *name, unsigned int lineno, const char *msg);

/// @brief Fill parsing session name and starting line.
void startparsing(const char *name, unsigned int start_line);

/// @brief Register error callback, returns old callback.
parse_err_callback_t setperrcallback(parse_err_callback_t cb);

/// @brief Return next token, \a NULL on end of parse.
char *parse(FILE *f);

/// @brief Place token back into the stream.
void ungettoken(const char *tok);

/// @brief Expect a token (\a NULL to expect any token).
char *expecttoken(FILE *f, const char *what);

/// @brief Expect integer.
int iexpecttoken(FILE *f);

/// @brief Expect floating point value.
double fexpecttoken(FILE *f);

/// @brief Skip remaining tokens in this line.
void skiptonextline(FILE *f);

/// @brief Trigger a parsing error at the current position.
void parsingerr(const char *msg, ...);

/** @} */

#endif
