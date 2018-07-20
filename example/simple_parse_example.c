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

#include <isolario/parse.h>
#include <isolario/progutil.h>
#include <stdio.h>
#include <string.h>
#include <stdnoreturn.h>
#include <stdlib.h>

static noreturn void usage(void)
{
    fprintf(stderr, "usage: %s FILE\n", programnam);
    exit(EXIT_FAILURE);
}

static noreturn void parsing_error(const char *name, unsigned int lineno, const char *msg)
{
    // just exit printing the error message
    exprintf(EXIT_FAILURE, "%s:%u: %s", name, lineno, msg);
}

// parse a single file given from program arguments,
// complete with error checking.
// Prints out parsed tokens to stdout, one per line
int main(int argc, char **argv)
{
    setprogramnam(argv[0]);

    if (argc != 2)
        usage();

    FILE *f;
    if (strcmp(argv[1], "-") == 0) {
        // allow parsing from stdin
        argv[1] = "(stdin)";
        f = stdin;
    } else {
        f = fopen(argv[1], "r");
    }
    if (!f)
        exprintf(EXIT_FAILURE, "could not open '%s':", argv[1]);

    // register parsing session informations and error callback,
    // this is important to achieve good and more informative error messages
    // in callback, we pass 0 as the first line, which is the 'default'
    // (actually a synonym for 1), we could start from the middle of the file
    // and adjust this value to reflect the first line
    startparsing(argv[1], 0);
    // save the old error-handling function to restore it later,
    // if we know that no handler was registered before, we can assume it to
    // be just NULL, we do it anyway in this example just for reference
    parse_err_callback_t old_handler = setperrcallback(parsing_error);

    const char *tok;
    while ((tok = parse(f)) != NULL)
        puts(tok);  // echo each token one per line

    // parse() shall return NULL on end of file,
    // but may also return NULL when a I/O error occurs,
    // we don't bother to handle I/O errors in any special way, so redirect
    // them to the usual parsing error path
    if (ferror(f))
        parsingerr("read error");

    // done parsing, close the file
    if (f != stdin)
        fclose(f);

    // ...and restore the old handler
    // (in this case we could have done a simple setperrcallback(NULL),
    // but we use the more general approach just for reference)
    setperrcallback(old_handler);

    // flush stdout to be sure no output error occurs,
    // in case we're writing to file
    if (fflush(stdout) != 0)
        exprintf(EXIT_FAILURE, "could not write to stdout:");

    return EXIT_SUCCESS;
}
