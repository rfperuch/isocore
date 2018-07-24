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

#include <errno.h>
#include <isolario/parse.h>
#include <isolario/progutil.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdnoreturn.h>
#include <stdlib.h>
#include <math.h>

// register index descriptor
typedef int regidx_t;

// simple opcodes to perform basic arithmetics
typedef enum {
    OP_BAD,

    OP_ADD, OP_SUB,
    OP_MUL, OP_DIV, OP_MOD,
    OP_POW,
    OP_LT, OP_LE, OP_EQ, OP_NE, OP_GT, OP_GE,
    OP_OR, OP_AND,

    OP_NUM
} opcode_t;

// expressions priority
typedef enum {
    EPRIO_NONE = 0,
    EPRIO_ARIT_SHORT,
    EPRIO_ARIT_LONG,
    EPRIO_ARIT_POW,
    EPRIO_BOOL,
    EPRIO_REL
} priority_t;

// simple operation descriptor
typedef struct {
    opcode_t opcode;   // opcode
    regidx_t a, b, r;  // first operand, second operand and result indexes
} op_t;

enum {
    REG_INVALID = -1,

    REGS_MAX = 256,
    EXPR_MAX = 512
};

typedef struct var_s var_t;  // forward declare for reg_t

typedef union {
    double val;
    var_t *var;
} reg_t;

struct var_s {
    struct var_s *next;

    // static state snapshot for variable evaluation
    int           opscount;
    char          name[TOK_LEN_MAX + 1];
    unsigned char varmask[REGS_MAX / 8 + 1];  // we only need the varsmap
    reg_t         regs[REGS_MAX];
    op_t          ops[];
};

// global jump buffer for parsing error
static jmp_buf errbuf;

static var_t *vars = NULL;                         // variable list
static op_t ops[EXPR_MAX];                         // current expression tree
static int  opscount = 0;                          // used operations count
static unsigned char constmask[REGS_MAX / 8 + 1];  // bitmask, 1 if register is constant
static unsigned char varmask[REGS_MAX / 8 + 1];    // bitmask, 1 if register contains a var_t*
static reg_t regs[REGS_MAX];                       // registers map
static int regscount = 0;                          // used registers count

static noreturn void usage(void)
{
    fprintf(stderr, "usage: %s FILE\n", programnam);
    exit(EXIT_FAILURE);
}

static void clearstate(void)
{
    memset(constmask, 0, sizeof(constmask));
    memset(varmask, 0, sizeof(varmask));
    opscount  = 0;
    regscount = 0;
}

static void freevars(void)
{
    while (vars) {
        var_t *tmp = vars;
        vars = tmp->next;

        free(tmp);
    }
}

static void parsing_error(const char *name, unsigned int lineno, const char *msg, void *data)
{
    FILE *f = data; // the handler receives the input stream (see startparsing())
    int c;

    eprintf("%s:%u: %s", name, lineno, msg);

    // flush input file
    do c = fgetc(f); while (c != EOF && c != '\n');

    // put the newline back (EOF has no effect)
    ungetc(c, f);

    // go to our safe parsing reset and attempt to continue parsing
    longjmp(errbuf, -1);
}

// Emit a register constant
static regidx_t emitconst(double val)
{
    // try to reuse an existing constant
    for (regidx_t i = 0; i < regscount; i++) {
        if ((constmask[i >> 3] & (1 << (i & 0x7))) == 0)
            continue;
        if (regs[i].val == val)
            return i;
    }

    // allocate a new constant
    if (regscount == REGS_MAX)
        parsingerr("operands hit REGS_MAX");

    regidx_t idx = regscount++;
    constmask[idx >> 3] |= 1 << (idx & 0x7);  // mark as const
    regs[idx].val = val;
    return idx;
}

// Emit a variable reference
static regidx_t emitvar(var_t *var)
{
    if (regscount == REGS_MAX)
        parsingerr("operands hit REGS_MAX");

    regidx_t idx = regscount++;
    varmask[idx >> 3] |= 1 << (idx & 0x7);
    regs[idx].var = var;
    return idx;
}

// emit a new operation
static regidx_t emitop(regidx_t a, opcode_t code, regidx_t b)
{
    if (opscount == EXPR_MAX)
        parsingerr("expressions hit EXPR_MAX");
    if (regscount == REGS_MAX)
        parsingerr("operands hit REGS_MAX");

    int idx = opscount++;
    ops[idx].opcode = code;
    ops[idx].a      = a;
    ops[idx].b      = b;
    ops[idx].r      = regscount++;  // placeholder for result
    return ops[idx].r;
}

// parse an opcode, returning its priority, the opcode and the actual token.
static const char *parse_op(FILE *f, priority_t *pdst, opcode_t *opdst)
{
    const char *tok = parse(f);
    opcode_t op  = OP_BAD;
    priority_t p = EPRIO_NONE;

    if (tok) {
        if (strcmp(tok, "+") == 0) {
            op = OP_ADD;
            p = EPRIO_ARIT_SHORT;
        } else if (strcmp(tok, "-") == 0) {
            op = OP_SUB;
            p = EPRIO_ARIT_SHORT;
        } else if (strcmp(tok, "*") == 0) {
            op = OP_MUL;
            p = EPRIO_ARIT_LONG;
        } else if (strcmp(tok, "/") == 0) {
            op = OP_DIV;
            p = EPRIO_ARIT_LONG;
        } else if (strcmp(tok, "%") == 0) {
            op = OP_MOD;
            p = EPRIO_ARIT_LONG;
        } else if (strcmp(tok, "^") == 0) {
            op = OP_POW;
            p = EPRIO_ARIT_POW;
        } else if (strcmp(tok, "<") == 0) {
            op = OP_LT;
            p = EPRIO_BOOL;
        } else if (strcmp(tok, "<=") == 0) {
            op = OP_LE;
            p = EPRIO_BOOL;
        } else if (strcmp(tok, "==") == 0) {
            op = OP_EQ;
            p = EPRIO_BOOL;
        } else if (strcmp(tok, "!=") == 0) {
            op = OP_NE;
            p = EPRIO_BOOL;
        } else if (strcmp(tok, ">=") == 0) {
            op = OP_GE;
            p = EPRIO_BOOL;
        } else if (strcmp(tok, ">") == 0) {
            op = OP_GT;
            p = EPRIO_BOOL;
        } else if (strcmp(tok, "||") == 0) {
            op = OP_OR;
            p = EPRIO_REL;
        } else if (strcmp(tok, "&&") == 0) {
            op = OP_AND;
            p = EPRIO_REL;
        }
    }

    if (pdst)
        *pdst = p;
    if (opdst)
        *opdst = op;

    return tok;
}

static regidx_t parse_expr(FILE *f);  // forward declare, for parse_term()

static regidx_t parse_term(FILE *f)
{
    regidx_t a, b, r;

    const char *tok = parse(f);
    if (!tok)
        parsingerr("unexpected end of expression");

    if (strcmp(tok, "(") == 0) {
        r = parse_expr(f);
        expecttoken(f, ")");
    } else if (strcmp(tok, "-") == 0 || strcmp(tok, "+") == 0) {
        char sign = tok[0];
        opcode_t op = (sign == '+')? OP_ADD : OP_SUB;
        a = emitconst(0);
        b = parse_expr(f);
        if (b == -1)
            parsingerr("unary %c with no argument", sign);

        r = emitop(a, op, b);
    } else {
        // is it a double?
        char *eptr;

        errno = 0;
        double v = strtod(tok, &eptr);
        if (tok != eptr) {
            if (errno != 0) {
                // use parsingerr() to automatically populate
                // the message with strerror(errno)
                parsingerr("'%s':", tok);
            }

            r = emitconst(v);
        } else {
            // variable reference?
            var_t *v;
            for (v = vars; v; v = v->next) {
                if (strcmp(v->name, tok) == 0)
                    break;
            }
            if (!v)
                parsingerr("unknown variable '%s'", tok);

            r = emitvar(v);
        }
    }

    return r;
}

static regidx_t parse_expr_rec(FILE *f, regidx_t a, priority_t prio)
{
    while (true) {
        opcode_t op;
        priority_t cur, next;

        const char* tok = parse_op(f, &cur, &op);
        if (!tok)
            break;
        if (cur == EPRIO_NONE || cur < prio) {
            ungettoken(tok);
            break;
        }

        regidx_t b = parse_term(f);
        while (true) {
            tok = parse_op(f, &next, NULL);
            if (!tok)
                break;

            ungettoken(tok);
            if (next == EPRIO_NONE || next <= cur)
                break;

            b = parse_expr_rec(f, b, next);
        }

        a = emitop(a, op, b);
    }

    return a;
}

// moves the expression tree in a variable.
static var_t *newvar(const char *name)
{
    for (var_t *i = vars; i != NULL; i = i->next) {
        if (strcmp(i->name, name) == 0)
            parsingerr("%s: variable already exists", name);
    }


    var_t *v = malloc(sizeof(*v) + opscount * sizeof(*ops));
    if (!v)
        exprintf(EXIT_FAILURE, "out of memory");

    v->opscount = opscount;
    strcpy(v->name, name);
    memcpy(v->varmask, varmask, ((regscount >> 3) + 1) * sizeof(*v->varmask));
    memcpy(v->regs, regs, regscount * sizeof(*v->regs));
    memcpy(v->ops,  ops,  opscount * sizeof(*v->ops));

    v->next = vars;
    vars    = v;
    return v;
}

// simple BCPL expression parser.
static regidx_t parse_expr(FILE *f)
{
    regidx_t a = parse_term(f);
    return parse_expr_rec(f, a, EPRIO_NONE);
}

// evaluate an expression tree
static double eval(const op_t *restrict ops, int opscount,
                   const unsigned char *restrict varmask,
                   reg_t *regs)
{
    const op_t *op = ops;
    for (int i = 0; i < opscount; i++, op++) {
        const reg_t *ra = &regs[op->a];
        const reg_t *rb = &regs[op->b];
        double a = ra->val;
        double b = rb->val;
        if (varmask[op->a >> 3] & (1 << (op->a & 0x7))) {
            // first operand is a variable, evaluate it
            var_t *v = ra->var;
            a = eval(v->ops, v->opscount, v->varmask, v->regs);
        }
        if (varmask[op->b >> 3] & (1 << (op->b & 0x7))) {
            // second operand is a variable, evaluate it
            var_t *v = rb->var;
            b = eval(v->ops, v->opscount, v->varmask, v->regs);
        }

        switch (op->opcode) {
        case OP_ADD:
            regs[op->r].val = a + b;
            break;
        case OP_SUB:
            regs[op->r].val = a - b;
            break;
        case OP_MUL:
            regs[op->r].val = a * b;
            break;
        case OP_DIV:
            regs[op->r].val = a / b;
            break;
        case OP_MOD:
            regs[op->r].val = fmod(a, b);
            break;
        case OP_POW:
            regs[op->r].val = pow(a, b);
            break;
        case OP_LT:
            regs[op->r].val = a < b;
            break;
        case OP_LE:
            regs[op->r].val = a <= b;
            break;
        case OP_EQ:
            regs[op->r].val = a == b;
            break;
        case OP_NE:
            regs[op->r].val = a != b;
            break;
        case OP_GT:
            regs[op->r].val = a > b;
            break;
        case OP_GE:
            regs[op->r].val = a >= b;
            break;
        case OP_OR:
            regs[op->r].val = a != 0.0 || b != 0.0;
            break;
        case OP_AND:
            regs[op->r].val = a != 0.0 && b != 0.0;
            break;
        default:
            exprintf(EXIT_FAILURE, "unknown operator: %#x", (unsigned int) op->opcode);
            return REG_INVALID;
        }
    }

    op--;
    return regs[op->r].val;
}

int main(int argc, char **argv)
{
    setprogramnam(argv[0]);

    if (argc != 2)
        usage();

    FILE *f = fopen(argv[1], "r");
    if (!f)
        exprintf(EXIT_FAILURE, "could not open '%s':", argv[1]);

    // pass the input file to the error handler, so we can flush it
    startparsing(argv[1], 0, f);

    parse_err_callback_t old_handler = setperrcallback(parsing_error);

    const char *tok;
    char varname[TOK_LEN_MAX + 1];

    // this is our safe spot to retry parsing, tolerating errors
    setjmp(errbuf);

    while (true) {
        clearstate();

        tok = parse(f);
        if (!tok || strcmp(tok, "quit") == 0)
            break;     // quit or EOF exits the sample program
        if (tok[0] == '\0')
            continue;  // skip empty lines (useful for stdin)

        if (strcmp(tok, "var") == 0) {
            // variable declaration:
            // var <name> = expr
            tok = expecttoken(f, NULL);
            // NOTE: be sure to copy the variable name, otherwise it would
            // be clobbered by the next call to parse()
            strcpy(varname, tok);

            expecttoken(f, "=");

            // expect an expression...
            parse_expr(f);

            // save state in a variable
            newvar(varname);
        } else {
            // just an expression, evaluate it and print it back
            ungettoken(tok);
            parse_expr(f);

            double res = eval(ops, opscount, varmask, regs);
            printf("%g\n", res);
        }
    }

    if (ferror(f))
        parsingerr("read error");

    // cleanup and exit
    freevars();
    if (f != stdin)
        fclose(f);

    setperrcallback(old_handler);

    if (fflush(stdout) != 0)
        exprintf(EXIT_FAILURE, "could not write to stdout:");

    return EXIT_SUCCESS;
}
