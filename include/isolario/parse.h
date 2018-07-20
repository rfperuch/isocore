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

#include <isolario/funcattribs.h>
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
 * * Tokens - A token is any word separated by whitespaces, a number of escape
 *            sequences can be used to express special caracters inside them.
 *
 * * Comments - A hash (\a #) starts a comment that extends to the end of
 *              the line it appears in; \a # is a reserved character and
 *              it may not be used inside a token. A comment may appear
 *              anywhere in text.
 *
 * The parser recognizes the following escape sequences:
 * * '\\n'  - newline
 * * '\\v'  - vertical tab
 * * '\\t'  - horizontal tab
 * * '\\r'  - caret return
 * * '\\#'  - hash character
 * * '\\\\' - backslash character
 * * '\\ '  - space character
 * * '\\n'  - token follows after newline
 *
 * Error handling is performed via callbacks, an error callback may be
 * registered to handle parsing errors, when such error is encountered,
 * a callback is invoked with a human readable message, when available,
 * the current parsing session name and line number are also provided.
 * The user is responsible to terminate parsing under such circumstances,
 * typically by using \a longjmp() or similar methods.
 *
 * @note Since the most effective way to terminate parsing is using longjmp(),
 *       extra care should be taken to avoid using Variable Length Arrays (VLA)
 *       during parsing operation, since doing so may cause memory leaks.
 *
 * @{
 */

enum {
    TOK_LEN_MAX = 256 ///< Maximum token length.
};

/**
 * @brief Error handling callback for parser.
 *
 * Defines a pointer to function with the signature expected by the parser
 * whenever a parsing-error handler is called.
 *
 * @param [in] name   The parsing session name, as specified by \a startparsing().
 *                    This argument may be \a NULL when no parsing session
 *                    information is available (e.g. \a startparsing() wasn't called).
 * @param [in] lineno The line number in which error was detected,
 *                    the handler may discretionally ignore this argument if
 *                    \a name is \a NULL (for example it may make little sense
 *                    to print a line number information when parsing from a
 *                    console).
 * @param [in] msg    An informative human readable parsing error message,
 *                    the handler shall never be called by the parser with
 *                    a \a NULL value for this argument.
 *
 * @see setperrcallback()
 */
typedef void (*parse_err_callback_t)(const char *name, unsigned int lineno, const char *msg);

/// @brief Fill parsing session name and starting line.
void startparsing(const char *name, unsigned int start_line);

/**
 * @brief Register a parsing error callback, returns old callback.
 *
 * Whenever the \a parse() function (or any other parsing function) encounters
 * a parsing error, the provided function is called with a meaningful context
 * information to handle the parsing error. Typically the application
 * wants to print or store the parsing error for logging or notification
 * purposes.
 *
 * The registered function is responsible to decide whether the application
 * may recover from a parsing error or not.
 * If the application does not tolerate parsing errors at all, the most
 * effective way to do so is calling \a exit() or similar functions to
 * terminate execution from within the handler, doing so will keep
 * the parsing logic clean.
 * If the application is capable of tolerating parsing errors, then
 * there are two approaches to do so:
 * * The handler can return normally to the parer, after possibly setting
 *   some significant variable, and the parser will return special values to
 *   its caller (e.g. \a parse() will return \a NULL, as if end-of-parse
 *   occurred), the application is thus resposible of doing the usual error
 *   checking.
 * * The handler may use \a longjmp() to jump back to a known \a setjmp()
 *   buffer, the application handles this as a parsing error and completely
 *   halts the parsing process, performing any required action.
 *   In this scenario the handler never returns to the parser and
 *   errors can be dealt with in a fashion similar to an "exception".
 *   Be careful when using this approach with Variable Length Arrays, since
 *   it may introduce unobvious memory-leak.
 *
 * @param [in] cb Function to be called whenever a parsing error occurs.
 *                Specify \a NULL to remove any installed handler.
 *
 * @return The previously registered error handler, \a NULL if no handler
 *         was registered.
 *
 * @see startparsing()
 */
parse_err_callback_t setperrcallback(parse_err_callback_t cb);

/**
 * @brief Return next token, or \a NULL on end of parse.
 *
 * This function should be called repeatedly on an input \a FILE to
 * parse each token, on end-of-file \a NULL will be returned.
 *
 * @param [in] f The input file to be parsed, this function does not impose
 *               the caller to provide the same source for each call,
 *               you may freely change the source. This argument must not be
 *               \a NULL.
 *
 * @return A pointer to a statically (thread-local) allocated storage,
 *         that is guaranteed to remain valid at least up to the next call
 *         to this function. The returned pointer must not be \a free()d.
 *
 * @note On read error, \a NULL is returned, as if EOF was encountered,
 *       the caller may distinguish such situations by invoking \a ferror()
 *       on \a f.
 */
nonnull(1) char *parse(FILE *f);

/**
 * @brief Place token back into the stream.
 *
 * This function makes possible to place back a parsed token into
 * the parsing stream, the next parsing call shall return it as if it
 * was encountered for the first time.
 *
 * @param [in] tok The token to be placed back into the parser,
 *                 this argument can be \a NULL, and if it is, this function
 *                 has no effect.
 *                 Behavior is undefined if \a tok is not \a NULL and it was
 *                 not returned by a previous call to \a parse() or any other
 *                 parsing function.
 *
 */
void ungettoken(const char *tok);

/**
 * @brief Expect a token (\a NULL to expect any token).
 *
 * Behaves like a call to \a parse(), but additionally requires a token
 * to exist in the input source.
 * This function shall consider an end of parse condition as a parsing error,
 * and call the parsing error handler registered by the latest call to
 * \a setperrcallback() with the appropriate arguments.
 * If the \a what argument is not \a NULL, then this function also ensures
 * that the token matches exactly (as in \a strcmp()) the provided argument
 * string.
 *
 * @param [in] f    The input source, must not be \a NULL.
 *                  The same considerations as \a parse() hold valid for this
 *                  function.
 * @param [in] what An optional string that requires the next token to match
 *                  exactly this string. If this argument is not \a NULL, then
 *                  encountering a token which does not compare as equal to
 *                  this string is considered a parsing error.
 *
 * @return The next token in input source, \a NULL on error: in particular,
 *         if no error handler is currently registered, \a NULL is returned
 *         to signal an unexpected end of parse or token mismatch.
 *         The same storage considerations as \a parse() hold valid for the
 *         returned buffer.
 */

nonnull(1) char *expecttoken(FILE *f, const char *what);

/**
 * @brief Expect integer value.
 *
 * Behaves like \a expecttoken(), but additionally considers a parsing error
 * to encounter a token which is not an invalid or out of range base 10 integer
 * value.
 *
 * @param [in] f The input source, the same considerations as \a parse()
 *               hold valid for this function.
 *
 * @return The parsed integer token, 0 is returned on parsing error.
 *
 * @note Since 0 is a valid integer token, the error handler should take
 *       action to inform the caller that a bad token was encountered to
 *       disambiguate this event.
 *
 * @see setperrcallback()
 */
nonnull(1) int iexpecttoken(FILE *f);

/// @brief Behaves like \a iexpecttoken(), but allows for \a long \a long values.
nonnull(1) long long llexpecttoken(FILE *f);

/**
 * @brief Expect floating point value.
 *
 * Behaves like \a expecttoken(), but additionally considers a parsing error
 * to encounter a token which is not an invalid floating point value.
 *
 * @param [in] f The input source, the same considerations as \a parse()
 *               hold valid for this function.
 *
 * @return The parsed floating point token, 0.0 is returned on parsing error.
 *
 * @note Since 0.0 is a valid floating point token, the error handler should
 *       take action to inform the caller that a bad token was encountered to
 *       disambiguate this event.
 *
 * @see setperrcallback()
 */
nonnull(1) double fexpecttoken(FILE *f);

/**
 * @brief Skip remaining tokens in this line.
 *
 * Advances until a newline character is encountered, effectively skipping
 * any token in the current line.
 * Any token fed to \a ungettoken() is discarded.
 *
 * @param [in] f The input source, must not be \a NULL.
 *               The same considerations as \a parse() hold valid for this
 *               function.
 *
 * @note In the event that a token spans over multiple lines (e.g. it contains the
 *       '\\n' escape sequence), such token is considered to be in the next line,
 *       so that same token will be the one returned by the subsequent call to
 *       \a parse().
 */
nonnull(1) void skiptonextline(FILE *f);

/**
 * @brief Trigger a parsing error at the current position.
 *
 * This function can be called to signal a parsing error at the current
 * position, it is especially useful when performing additional checks
 * on token returned by this API.
 * The message is formatted in a printf-like fashion.
 *
 * @param [in] msg Parsing error format string.
 * @param [in] ... Format arguments.
 *
 * @see setperrcallback()
 */
printflike(1, 2) nonnull(1) void parsingerr(const char *msg, ...);

/**
 * @example simple_parse_example.c
 *
 * This is a simple example for the \a parse() function and the overall
 * @ref Parse API, including thorough error handling.
 * It demonstrates the intended API usage model.
 */

/** @} */

#endif
