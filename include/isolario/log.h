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
 */

#ifndef ISOLARIO_LOG_H_
#define ISOLARIO_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup Logging Isolario logging facility
 *
 * @brief Logging functions for the Isolario.
 *
 * @{
 */

/**
 * @brief Log severity enumeration.
 *
 * @see loglevel
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
 * @see logopen
 */
enum {
    lmodecol   = 1 << 0,  ///< Enable colors when logging to terminal.
    lmodecreat = 1 << 1,  ///< Do not append to file, rather truncate it.
    lmodenocon = 1 << 2,  ///< Do not log to console.
    lmodesync  = 1 << 3,  ///< Enable sync write to log file.
    lmodestamp = 1 << 4   ///< Prefix a timestamp to every log entry.
};

/** @brief Set logging minimum severity level, messages lower than \a sev are discarded. */
logsev_t loglevel(logsev_t sev);
/** @brief Direct logging messages to other files. */
int logopen(const char *logfile, int mode);
/** @brief Log a message. */
void logprintf(logsev_t sev, const char *fmt, ...);
/** @brief Close logging system resources. */
void logclose(void);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
