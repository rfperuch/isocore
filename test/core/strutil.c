#include <CUnit/CUnit.h>
#include <isolario/strutil.h>
#include <isolario/util.h>
#include <stdlib.h>
#include <test_util.h>

#include "test.h"

void testtrimwhites(void)
{
    const struct {
        const char *input;
        const char *expect;
    } table[] = {
        {
            "nowhites",
            "nowhites"
        }, {
            "",
            ""
        }, {
            "           ",
            ""
        }, {
            "     onlyleading",
            "onlyleading"
        }, {
            "onlytrailing     ",
            "onlytrailing"
        }, {
            "     both       ",
            "both"
        }, {
            " mixed inside string too     ",
            "mixed inside string too"
        }
    };
    for (size_t i = 0; i < nelems(table); i++) {
        char buf[strlen(table[i].input) + 1];

        strcpy(buf, table[i].input);
        CU_ASSERT_STRING_EQUAL(trimwhites(buf), table[i].expect);
    }
}


void testjoinstrv(void)
{
    char *joined = joinstrv(" ", "a", "fine", "sunny", "day", (char *) NULL);
    CU_ASSERT_STRING_EQUAL(joined, "a fine sunny day");
    free(joined);

    joined = joinstrv(" not ", "this is", "funny", (char *) NULL);
    CU_ASSERT_STRING_EQUAL(joined, "this is not funny");
    free(joined);

    joined = joinstrv(" ", (char *) NULL);
    CU_ASSERT_STRING_EQUAL(joined, "");
    free(joined);

    joined = joinstrv(" ", "trivial", (char *) NULL);
    CU_ASSERT_STRING_EQUAL(joined, "trivial");
    free(joined);

    joined = joinstrv("", "no", " changes", " to", " be", " seen", " here", (char *) NULL);
    CU_ASSERT_STRING_EQUAL(joined, "no changes to be seen here");
    free(joined);

    joined = joinstrv(NULL, "no", " changes", " here", " either", (char *) NULL);
    CU_ASSERT_STRING_EQUAL(joined, "no changes here either");
    free(joined);
}

void testsplitjoinstr(void)
{
    const struct {
        const char *input;
        const char *delim;
        size_t n;
        const char *expected[16];
    } table[] = {
        {
            "a whitespace separated string",
            " ", 4,
            { "a", "whitespace", "separated", "string" }
        }, {
            "",
            NULL, 0
        }, {
            "",
            "", 0
        }
    };
    for (size_t i = 0; i < nelems(table); i++) {
        size_t n;
        char **s = splitstr(table[i].input, table[i].delim, &n);

        CU_ASSERT_EQUAL_FATAL(n, table[i].n);
        for (size_t j = 0; j < n; j++)
            CU_ASSERT_STRING_EQUAL(s[j], table[i].expected[j]);

        CU_ASSERT_PTR_EQUAL(s[n], NULL);

        char *sj = joinstr(table[i].delim, s, n);
        CU_ASSERT_STRING_EQUAL(table[i].input, sj);

        free(s);
        free(sj);
    }
}

