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
 * @file isolario/hexdump.h
 *
 * @brief Utilities to dump memory chunks into human readable hex dumps.
 *
 * @note This file is guaranteed to include standard \a stdio.h, a number of
 *       other standard or isolario-specific file may be included in the
 *       interest of providing additional functionality, but such inclusion
 *       should not be relied upon by the includers of this file.
 */

#ifndef ISOLARIO_HEXDUMP_H_
#define ISOLARIO_HEXDUMP_H_

#include <stdio.h>

/**
 * @defgroup Core_hexdump Human readable binary data dump
 *
 * @brief Functions to format byte chunks into readable strings in various
 *        binary encoding formats.
 *
 * Binary formatting is controlled by a mode string which determines the desired
 * encoding, in a way similar to standard printf().
 * The general mode string format is defined as follows:
 *
 * @verbatim [Fmt][#][Sep][Group][Sep][Cols] @endverbatim
 *
 * With the following semantics:
 * * Fmt   - The desired binary format, valid values are:
 *           * \a 'x' - Hexadecimal notation.
 *           * \a 'X' - Hexadecimal notation, with uppercase letters.
 *           * \a 'o' - Octal notation.
 *           * \a 'O' - Synonym with \a o.
 *           * \a 'b' - Binary notation, prints each byte as 8 bits.
 *           * \a 'B' - Binary notation, this is effectively a synonym
 *                      with \a b, but the format may be altered by
 *                      the \a alternative formatting flag, see below.
 *
 *           If nothing is specified, the default format is used, which is \a x.
 *
 * * #     - Alternative formatting flag, each byte group is prepended with a
 *           sequence explicitly indicating the encoding format specified in
 *           \a Fmt. The exact formatting output depends on the \a Fmt value
 *           itself and is defined as follows:
 *           * \a 'x' - Each byte group is prepended by a \a 0x, indicating
 *                      their hexadecimal encoding.
 *           * \a 'X' - Uppercase variant of \a x, uses a \a 0X prefix for each
 *                      group.
 *           * \a 'o' - Each group is prepended by a 0, indicating their octal
 *                      encoding.
 *           * \a 'O' - Synonym with \a o.
 *           * \a 'b' - Each byte group is prepended by a \a b, indicating their
 *                      binary encoding.
 *           * \a 'B' - Uppercase variant of \a b, uses a \a B prefix for each
 *                      group.
 *
 * * Sep   - Group separator specifier, by default, groups are not separated in
 *           any way, the resulting output is a single string in the format
 *           specified by \a Fmt. This behavior can be altered with a separator
 *           marker, the accepted separator characters are:
 *           * \a ' ' - Add a space between each group.
 *           * \a ',' - Add comma followed by a space between each group.
 *           * \a '/' - Synonym with \a , for readability only.
 *           * \a '|' - Add a space followed by a pipe between each group.
 *           * \a '(' - Prepend a bracket before the whole output, and
 *                      close it at the end, each group is separated by a comma
 *                      as if by \a ','.
 *           * \a '[' - Same as \a '(', but uses a square bracket.
 *           * \a '{' - Same as \a '(', but uses a curly bracket.
 *
 *           A separator should be followed by the requested size constraints
 *           defined by \a Group and \a Cols, described below.
 *
 * * Group - An unsigned integer value specifying how many bytes should be
 *           grouped toghether when formatting them, when the value is 1, each
 *           byte is printed separately; when the value is 2, bytes pair of
 *           bytes are coupled toghether when printing them, and so on...
 *           By default all bytes are taken as a single group and are not
 *           separated in any way, *most likely, when a \a Sep is specified
 *           in the mode string, this value should also be specified, otherwise
 *           the \a Sep modifier has no effect*.
 *           As an additional feature, the \a '*' value can be used for this
 *           field, in this case, the group lenght is taken from the next
 *           *integer* (that is: \a int) variable in the variable argument list.
 *
 * * Sep   - This field introduces an additional constrain on the number of
 *           columns before a newline is issued by the formatter.
 *           The only use for this field is to separate the \a Group value from
 *           the \a Cols value, specified below.
 *           The specifier must match the first \a Sep.
 *           As an additional convenience to improve readability, when a paren
 *           is specified for the original \a Sep (either \a '(', \a '[',
 *           or \a '{'), the corresponding closing paren may be used for this
 *           field.
 *
 * * Cols  - An unsigned integer value specifying a hint for the desired column
 *           width for the produced string. Whenever this column limit is
 *           exceeded, a newline is inserted in place of the closest following
 *           whitespace character. Please note that this is not a mandatory
 *           limit, a group of bytes is never split in two lines, even when
 *           its size would exceed this value, and a comma or pipe character
 *           is never split on a new line.
 *           Like \a Group, this field can also be specified inside the argument
 *           list by a corresponding *integer* variable (that is: \a int), if
 *           a \a '*' is specified for this field.
 *
 * @{
 */

/**
 * @brief Formatting mode to obtain a valid C array of hexadecimal bytes,
 *        the array is formatted in a single line.
 *
 * @see hexdump(), hexdumps()
 */
#define HEX_C_ARRAY "x#{1}"

/**
 * @brief Formatting mode to obtain a valid C array of hexadecimal bytes,
 *        an attempt is made to keep the array within a specific column limit.
 *
 * @param cols Integer column limit hint.
 *
 * @warning This macro should only be used as a \a mode argument of hexdump() or
 *          hexdumps()
 */
#define HEX_C_ARRAY_N(cols) "x#{1}*", (int) (cols)

/**
 * @brief Formatting mode to obtain a valid C array of octal bytes,
 *        the array is formatted in a single line.
 *
 * @see hexdump(), hexdumps()
 */
#define OCT_C_ARRAY "o#{1}"

/**
 * @brief Formatting mode to obtain a valid C array of octal bytes,
 *        an attempt is made to keep the array within a specific column limit.
 *
 * @param cols Integer column limit hint.
 *
 * @warning This macro should only be used as a \a mode argument of hexdump() or
 *          hexdumps()
 */
#define OCT_C_ARRAY_N(cols) "o#{1}*", (int) (cols)

/**
 * @brief Formatting mode for a comma-separated string of hexadecimal bytes
 *        prepended by the "0x" prefix.
 *
 * @see hexdump(), hexdumps()
 */
#define HEX_PLAIN "x#/1"

/**
 * @brief Formatting mode for a comma-separated string of octal bytes.
 *
 * @see hexdump(), hexdumps()
 */
#define OCT_PLAIN "o/1"

/**
 * @brief Formatting mode for a comma-separated string of bytes encoded into
 *        binary digits.
 *
 * @see hexdump(), hexdumps()
 */
#define BINARY_PLAIN "b/1"

/**
 * @brief Hexadecimal dump to file stream.
 *
 * Formats a data chunk to a standard \a FILE in a human readable binary dump,
 * as specified by a mode string.
 *
 * @param [out] out  \a FILE handle where the formatted string has to be
 *                   written, must not be \a NULL.
 * @param [in]  data Data chunk that has to be dumped, must reference at least
 *                   \a n bytes, may be \a NULL if \a n is 0.
 * @param [in]  n    Number of bytes to be formatted to \a out.
 * @param [in]  mode Mode string determining output format, may be \a NULL to
 *                   use default formatting (equivalent to empty string).
 * @param [in]  ...  Additional \a mode arguments, see \a Group and \a Cols
 *                   documentation of module's mode strings.
 *
 * @return The number of bytes successfully written to \a out, which shall not
 *         exceed \a n and may only be less than \a n in presence of an I/O
 *         error.
 */
size_t hexdump(FILE *out, const void *data, size_t n, const char *mode, ...);

/**
 * @brief Hexadecimal dump to char string.
 *
 * Formats a data chunk directly into a string as a human readable binary dump,
 * following the rules specified by a mode string.
 *
 * @param [out] dst  Destination string where the formatted string has to be
 *                   written, may be \a NULL only if \a size is 0.
 * @param [in]  size Maximum number of characters that may be written to \a dst,
 *                   this function shall never attempt to write more than
 *                   \a size characters to \a dst, this quantity includes the
 *                   terminating \a NUL (\a '\0') character. The resulting
 *                   string is always \a NUL-terminated, even when truncation
 *                   occurred, except when \a size is 0.
 * @param [in]  data Data chunk that has to be dumped, must reference at least
 *                   \a n bytes, may be \a NULL if \a n is 0.
 * @param [in]  n    Number of bytes in \a data to be formatted into \a dst.
 * @param [in]  mode Mode string determining output format, may be \a NULL to
 *                   use default formatting (equivalent to empty string).
 * @param [in]  ...  Additional \a mode arguments, see \a Group and \a Cols
 *                   documentation of module's mode strings.
 *
 * @return The number of characters necessary to hold the entire dump, including
 *         the terminating NUL (\a '\0') character. If the returned value is
 *         greater than \a size, then a truncation occurred, the caller may
 *         then allocate a buffer of the appropriate size and call this function
 *         again to obtain the entire dump.
 *         A return value of 0 indicates an out of memory error.
 *
 * @note The caller may be able to determine the appropriate size for the
 *       destination buffer by calling this function with a \a NULL destination
 *       and zero \a size in advance, using the returned value to allocate a
 *       large enough buffer. However, when trivial formatting is enough,
 *       it is possible to determine such size a-priori, for example a buffer
 *       of size 2 * \a n + 1 is always large enough for a contiguous
 *       hexadecimal dump with no hex prefix. Similar considerations can be done
 *       for simple space, comma or pipe separated dumps (for example
 *       allocating a possible upperbound of 4 * \a n + 1 \a char buffer).
 *       Obvious adjustments of these calculations have to be performed for
 *       binary and octal dumps, and when prefix or grouping is requested.
 *       Still, the afore mentioned approach is valuable for non-trivial
 *       formatting and has the obvious advantage of not requiring careful
 *       thinking on caller's behalf.
 */
size_t hexdumps(char *dst, size_t size, const void *data, size_t n, const char *mode, ...);

/** @} */

#endif

