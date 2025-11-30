#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <regex.h>

#ifdef MUSL_PREFIX
// Declare musl's prefixed functions
extern int musl_regcomp(regex_t *preg, const char *regex, int cflags);
extern int musl_regexec(const regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags);
extern void musl_regfree(regex_t *preg);
#define REGCOMP musl_regcomp
#define REGEXEC musl_regexec
#define REGFREE musl_regfree
#else
#define REGCOMP regcomp
#define REGEXEC regexec
#define REGFREE regfree
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
        if (len < 2) continue;

        // Split input: first byte is flags, rest is pattern
        int cflags = 0;
        if (buf[0] & 0x01) cflags |= REG_EXTENDED;
        if (buf[0] & 0x02) cflags |= REG_ICASE;
        if (buf[0] & 0x04) cflags |= REG_NEWLINE;
        if (buf[0] & 0x08) cflags |= REG_NOSUB;

        // Null-terminate the pattern
        size_t pattern_len = len - 1;
        char *pattern = malloc(pattern_len + 1);
        if (!pattern) continue;
        memcpy(pattern, buf + 1, pattern_len);
        pattern[pattern_len] = '\0';

        regex_t preg;
        int ret = REGCOMP(&preg, pattern, cflags);

        if (ret == 0) {
            // Try matching against a test string
            regmatch_t pmatch[10];
            REGEXEC(&preg, "test string hello world 12345", 10, pmatch, 0);
            REGEXEC(&preg, pattern, 10, pmatch, 0); // Match against itself
            REGFREE(&preg);
        }

        free(pattern);
    }

#ifndef __AFL_FUZZ_TESTCASE_LEN
    free(buf);
#endif
    return 0;
}
