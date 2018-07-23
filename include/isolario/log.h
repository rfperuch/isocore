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
 * @file isolario/log.h
 *
 * @brief Logging facility functions.
 *
 * @note This header is guaranteed to include standard \a stdarg.h,
 *       no other assumption should be made upon this header's inclusions.
 */

#ifndef ISOLARIO_LOG_H_
#define ISOLARIO_LOG_H_

#include <isolario/funcattribs.h>
#include <stdarg.h>

/**
 * @defgroup Logging Isolario logging facility
 *
 * @brief Logging functions for the Isolario Project.
 *
 * This module implements a thread-safe, performance oriented logging facility,
 * capable of logging both to the console using \a stderr and to a file.
 *
 * @{
 */

/**
 * @brief Log severity enumeration.
 *
 * @see loglevel()
 */
typedef enum {
    logall = -1,  ///< Not a valid enum, requires logging for any level.

    logdev,       ///< Log developer only message.
    loginfo,      ///< Log regular status message.
    logwarn,      ///< Log warning message.
    logerr,       ///< Log error message.

    lognone,      ///< Not a valid enum, discards logging for any level.
    logquery      ///< Not a valid enum, query current log level.
} logsev_t;

/**
 * @brief Log mode bitmask, defines logging system behavior.
 *
 * @see logopen()
 */
enum {
    lmodecol   = 1 << 0,  ///< Enable colors when logging to terminal.
    lmodecreat = 1 << 1,  ///< Do not append to file, rather truncate it.
    lmodenocon = 1 << 2,  ///< Do not log to console.
    lmodesync  = 1 << 3,  ///< Enable sync write to log file.
    lmodestamp = 1 << 4   ///< Prefix a timestamp to every log entry.
};

/**
 * @brief Get or set logging severity level.
 *
 * This function can be used to query or set the logging system severity level.
 * Each logging message has a severity associated with it, the system shall
 * discard any message with a severity lower than the current logging severity
 * level (for example a \a logdev message shall be discarded if the current
 * severity is set to \a loginfo).
 *
 * @param [in] sev If this argument ranges from \a logdev to \a logerr, then
 *                 it is interpreted as the desired minimum logging level to be
 *                 set, and this function shall set the minimum level to
 *                 that value. If this argument is one of the special values,
 *                 then the following behavior shall be taken:
 *                 * \a logall   - No message shall be ever discarded.
 *                 * \a lognone  - Every message shall be discarded.
 *                 * \a logquery - No change is made to the current severity
 *                                 level, making this call only a query of the
 *                                 current severity.
 *
 * @return The previous severity level (if this function is called with
 *         \a logquery, then this is also the current level).
 *
 * @note This function is thread-safe, the initial logging level is \a loginfo.
 */
logsev_t loglevel(logsev_t sev);
/**
 * @brief Setup logging system and output mode.
 *
 * This function performs setup, or alters an existing setup, in the modality
 * described by its argument.
 *
 * @param [in] logfile The logfile to direct logging to, it may be \a NULL if
 *                     no logging to file should take place.
 *                     Whenever a file is specified, an attempt is made to
 *                     open it in append mode, if the attempt succeeds a line
 *                     is printed with a timestamp indicating the start time of
 *                     the logging activity.
 * @param [in] mode    A mode mask, each bit indicates the intended logging
 *                     behavior. The following bits are accepted:
 *                     * \a lmodenocon - Disable logging to \a stderr, useful to
 *                                       enable logging to file only, note that
 *                                       when this flag is specified and \a logfile
 *                                      is \a NULL, logging is effectively disabled.
 *                     * \a lmodecol   - Print to \a stderr using VT100 colors,
 *                                       ignored if \a lmodenocon is specified.
 *                     * \a lmodecreat - Alters the usual behavior of appending
 *                                       to \a logfile on open, truncating the file
 *                                       if already existing. Ignored if
 *                                       \a logfile is \a NULL.
 *                     * \a lmodestamp - Prepend a timestamp to each log message.
 *
 * @note This function is *not* thread safe.
 *       The initial logging system setup is:
 *       * \a NULL \a logfile
 *       * \a mode is 0, prints to \a stderr without VT100 colors or timestamps.
 */
int logopen(const char *logfile, int mode);
/**
 * @brief Log a message.
 *
 * Send a message to the logging system with the specified severity.
 * If the severity is lower than the current logging system minimum severity,
 * then the message is discarded.
 * The message is formatted in a \a printf()-like fashion, with the added
 * convienence of automatically appending the result of \a strerror(\a errno)
 * when the format string terminates with ':'. A newline is automatically
 * appended to the resulting message.
 * A prefix may be added to the message as specified by \a logopen().
 *
 * @param [in] sev Message severity.
 * @param [in] fmt Message format string.
 * @param [in] ... Format message arguments.
 *
 * @note This function is thread safe.
 *
 * @see loglevel()
 */
printflike(2, 3) nonnull(2) void logprintf(logsev_t sev, const char *fmt, ...);
/**
 * @brief Log a message, \a va_list variant.
 *
 * Behaves like \a logprintf(), but takes a direct \a va_list.
 *
 * @param [in] sev Message severity.
 * @param [in] fmt Message format string.
 * @param [in] va  Format arguments \a va_list.
 *
 * @note This function is thread safe.
 */
nonnull(2) void logvprintf(logsev_t sev, const char *fmt, va_list va);
/**
 * @brief Close logging system files and resources.
 *
 * After calling this function, the logging system is restored to its
 * default state.
 */
void logclose(void);

/** @} */

#endif
