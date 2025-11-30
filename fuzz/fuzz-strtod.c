#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#ifdef MUSL_PREFIX
extern double musl_strtod(const char *nptr, char **endptr);
extern float musl_strtof(const char *nptr, char **endptr);
extern long double musl_strtold(const char *nptr, char **endptr);
extern long musl_strtol(const char *nptr, char **endptr, int base);
extern long long musl_strtoll(const char *nptr, char **endptr, int base);
extern unsigned long musl_strtoul(const char *nptr, char **endptr, int base);
extern unsigned long long musl_strtoull(const char *nptr, char **endptr, int base);
#define STRTOD musl_strtod
#define STRTOF musl_strtof
#define STRTOLD musl_strtold
#define STRTOL musl_strtol
#define STRTOLL musl_strtoll
#define STRTOUL musl_strtoul
#define STRTOULL musl_strtoull
#else
#define STRTOD strtod
#define STRTOF strtof
#define STRTOLD strtold
#define STRTOL strtol
#define STRTOLL strtoll
#define STRTOUL strtoul
#define STRTOULL strtoull
#endif

#ifdef __AFL_FUZZ_TESTCASE_LEN
__AFL_FUZZ_INIT();
#endif

int main(int argc, char **argv) {
#ifdef __AFL_FUZZ_TESTCASE_LEN
    __AFL_INIT();
    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;

    while (__AFL_LOOP(10000)) {
        size_t len = __AFL_FUZZ_TESTCASE_LEN;
#else
    unsigned char *buf = NULL;
    size_t len = 0;

    if (argc > 1) {
        FILE *f = fopen(argv[1], "rb");
        if (!f) return 1;
        fseek(f, 0, SEEK_END);
        len = ftell(f);
        fseek(f, 0, SEEK_SET);
        buf = malloc(len + 1);
        if (!buf) { fclose(f); return 1; }
        fread(buf, 1, len, f);
        fclose(f);
    } else {
        return 1;
    }
    {
#endif
        if (len == 0) continue;

        // Null-terminate the input
        char *input = malloc(len + 1);
        if (!input) continue;
        memcpy(input, buf, len);
        input[len] = '\0';

        char *endptr;

        // Test strtod
        volatile double d = STRTOD(input, &endptr);
        (void)d;

        // Test strtof
        volatile float f = STRTOF(input, &endptr);
        (void)f;

        // Test strtold
        volatile long double ld = STRTOLD(input, &endptr);
        (void)ld;

        // Test strtol variants
        volatile long l = STRTOL(input, &endptr, 0);
        (void)l;

        volatile long long ll = STRTOLL(input, &endptr, 0);
        (void)ll;

        volatile unsigned long ul = STRTOUL(input, &endptr, 0);
        (void)ul;

        volatile unsigned long long ull = STRTOULL(input, &endptr, 0);
        (void)ull;

        // Test with different bases
        for (int base = 2; base <= 36; base++) {
            volatile long lb = STRTOL(input, &endptr, base);
            (void)lb;
        }

        free(input);
    }

#ifndef __AFL_FUZZ_TESTCASE_LEN
    free(buf);
#endif
    return 0;
}
