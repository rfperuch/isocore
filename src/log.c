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

#include <ctype.h>
#include <errno.h>
#include <isolario/branch.h>
#include <isolario/log.h>
#include <isolario/vt100.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static FILE *log_output = NULL;
static int log_mode = 0;
static atomic_int minlevel = loginfo;

static void clearvtcodes(char *s)
{
    char c;
    while ((c = *s) != '\0') {
        if (c != '\x1b') {
            s++;
            continue;
        }

        char *dst = s++;
        if (*s == '[' || *s == '(') {
            s++;
            while ((c = *s++) != '\0') {
                if (isalpha(c))
                    break;
            }
        }
        s = memmove(dst, s, strlen(s) + 1);
    }
}

static const char *sev2str(logsev_t sev)
{
    static const char *const strtab[] = {
        [logdev]  = "[DEBUG]  ",
        [loginfo] = "[" VTGRN "INFO" VTRST "]   ",
        [logwarn] = "[" VTYLW "WARNING" VTRST "]",
        [logerr]  = "[" VTRED "ERROR" VTRST "]  "
    };

    if (sev <= logall || sev >= lognone)
        sev = loginfo;

    return strtab[sev];
}

logsev_t loglevel(logsev_t sev)
{
    if (sev < logall || sev > lognone)
        return atomic_load(&minlevel);

    return atomic_exchange_explicit(&minlevel, sev, memory_order_relaxed);
}

int logopen(const char *logfile, int mode)
{
    log_mode = mode;
    if (logfile) {
        const char *modes = "a";
        if (mode & lmodecreat)
            modes = "w";

        log_output = fopen(logfile, modes);
        if (unlikely(!log_output))
            return -1;

        if (mode & lmodesync)
            setbuf(log_output, NULL);

        time_t t = time(NULL);
        fprintf(log_output, "opening log file on %s", asctime(gmtime(&t)));
    }
    return 0;
}

void logvprintf(logsev_t sev, const char *fmt, va_list va)
{
    if (sev < logdev || sev > logerr)
        return;  // ignore special values
    if (sev < loglevel(logquery))
        return;  // not logging on this severity
    if (!log_output && (log_mode & lmodenocon))
        return;  // no log output

    va_list cpy;
    va_copy(cpy, va);
    int n = vsnprintf(NULL, 0, fmt, cpy);
    va_end(cpy);

    if (n < 1)
        return;

    char stamp[64] = "";
    if (log_mode & lmodestamp) {
        struct timespec ts;
        timespec_get(&ts, TIME_UTC);

        struct tm *tm = gmtime(&ts.tv_sec);
        sprintf(stamp, "%02d.%02d.%02d-%02d:%02d:%02d.%09ld - ",
                1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec,
                ts.tv_nsec / 10000000
        );
    }

    const char *prefix = sev2str(sev);
    n += strlen(stamp) + strlen(prefix) + 2; // for ": " after prefix

    const char *err = NULL;
    if (fmt[strlen(fmt) - 1] == ':') {
        err = strerror(errno);
        n += strlen(err) + 1; // format ": <error string>"
    }

    n += strlen(VTRST) + 2;  // for "VTRST\n\0"

    char buf[n];

    buf[0] = '\0';
    strcat(buf, stamp);
    strcat(buf, prefix);
    strcat(buf, ": ");
    vsprintf(buf + strlen(buf), fmt, va);
    if (err) {
        strcat(buf, " ");
        strcat(buf, err);
    }
    strcat(buf, VTRST "\n");

    bool cleared = false;
    if ((log_mode & lmodecol) == 0) {
        clearvtcodes(buf);
        cleared = true;
    }

    if ((log_mode & lmodenocon) == 0)
        fprintf(stderr, "%s", buf);

    if (log_output) {
        if (!cleared)
            clearvtcodes(buf);

        fprintf(log_output, "%s", buf);
    }
}

void logprintf(logsev_t sev, const char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    logvprintf(sev, fmt, va);
    va_end(va);
}

void logclose(void)
{
    if (log_output) {
        time_t t = time(NULL);
        fprintf(log_output, "closing log file on %s", asctime(gmtime(&t)));
        fclose(log_output);

        log_output = NULL;
        log_mode = 0;
    }
}
