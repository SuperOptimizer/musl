#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#ifdef MUSL_PREFIX
extern int musl_snprintf(char *str, size_t size, const char *format, ...);
#define SNPRINTF musl_snprintf
#else
#define SNPRINTF snprintf
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

        // Null-terminate the format string
        char *fmt = malloc(len + 1);
        if (!fmt) continue;
        memcpy(fmt, buf, len);
        fmt[len] = '\0';

        // Output buffer (discard output)
        char output[4096];

        // Test snprintf with various argument types
        SNPRINTF(output, sizeof(output), fmt,
            42,                    // int
            3.14159,               // double
            "hello",               // string
            (void*)0x1234,         // pointer
            (long long)123456789,  // long long
            (unsigned)0xdeadbeef,  // unsigned
            'X',                   // char
            -42                    // negative int
        );

        free(fmt);
    }

#ifndef __AFL_FUZZ_TESTCASE_LEN
    free(buf);
#endif
    return 0;
}
