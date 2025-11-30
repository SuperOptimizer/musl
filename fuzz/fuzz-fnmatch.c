#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fnmatch.h>

#ifdef MUSL_PREFIX
extern int musl_fnmatch(const char *pattern, const char *string, int flags);
#define FNMATCH musl_fnmatch
#else
#define FNMATCH fnmatch
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
        if (len < 3) continue;

        // First byte is flags
        int flags = 0;
        if (buf[0] & 0x01) flags |= FNM_NOESCAPE;
        if (buf[0] & 0x02) flags |= FNM_PATHNAME;
        if (buf[0] & 0x04) flags |= FNM_PERIOD;
#ifdef FNM_CASEFOLD
        if (buf[0] & 0x08) flags |= FNM_CASEFOLD;
#endif

        // Split remaining into pattern and string
        size_t remaining = len - 1;
        size_t pattern_len = remaining / 2;
        size_t string_len = remaining - pattern_len;

        char *pattern = malloc(pattern_len + 1);
        char *string = malloc(string_len + 1);
        if (!pattern || !string) {
            free(pattern);
            free(string);
            continue;
        }

        memcpy(pattern, buf + 1, pattern_len);
        pattern[pattern_len] = '\0';

        memcpy(string, buf + 1 + pattern_len, string_len);
        string[string_len] = '\0';

        // Test fnmatch
        FNMATCH(pattern, string, flags);

        // Also test with fixed strings against fuzzed pattern
        FNMATCH(pattern, "hello.txt", flags);
        FNMATCH(pattern, "/usr/local/bin/test", flags);
        FNMATCH(pattern, ".hidden", flags);

        free(pattern);
        free(string);
    }

#ifndef __AFL_FUZZ_TESTCASE_LEN
    free(buf);
#endif
    return 0;
}
