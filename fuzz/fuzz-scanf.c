#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#ifdef MUSL_PREFIX
extern int musl_sscanf(const char *str, const char *format, ...);
#define SSCANF musl_sscanf
#else
#define SSCANF sscanf
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

        // Split input: first half is format, second half is input string
        size_t fmt_len = len / 2;
        size_t input_len = len - fmt_len;

        char *fmt = malloc(fmt_len + 1);
        char *input = malloc(input_len + 1);
        if (!fmt || !input) {
            free(fmt);
            free(input);
            continue;
        }

        memcpy(fmt, buf, fmt_len);
        fmt[fmt_len] = '\0';

        memcpy(input, buf + fmt_len, input_len);
        input[input_len] = '\0';

        // Output variables
        int i1 = 0, i2 = 0;
        double d1 = 0.0;
        char s1[256] = {0};
        long long ll1 = 0;
        unsigned u1 = 0;

        // Test sscanf
        SSCANF(input, fmt, &i1, &d1, s1, &ll1, &u1, &i2);

        free(fmt);
        free(input);
    }

#ifndef __AFL_FUZZ_TESTCASE_LEN
    free(buf);
#endif
    return 0;
}
