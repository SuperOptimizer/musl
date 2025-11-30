#define _GNU_SOURCE
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#ifdef MUSL_PREFIX
extern char *musl_strptime(const char *s, const char *format, struct tm *tm);
extern size_t musl_strftime(char *s, size_t max, const char *format, const struct tm *tm);
#define STRPTIME musl_strptime
#define STRFTIME musl_strftime
#else
#define STRPTIME strptime
#define STRFTIME strftime
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

        // Split input: first half is format, second half is time string
        size_t fmt_len = len / 2;
        size_t time_len = len - fmt_len;

        char *fmt = malloc(fmt_len + 1);
        char *timestr = malloc(time_len + 1);
        if (!fmt || !timestr) {
            free(fmt);
            free(timestr);
            continue;
        }

        memcpy(fmt, buf, fmt_len);
        fmt[fmt_len] = '\0';

        memcpy(timestr, buf + fmt_len, time_len);
        timestr[time_len] = '\0';

        struct tm tm = {0};
        char *ret = STRPTIME(timestr, fmt, &tm);

        // If parsing succeeded, also test strftime
        if (ret) {
            char output[256];
            STRFTIME(output, sizeof(output), fmt, &tm);
        }

        free(fmt);
        free(timestr);
    }

#ifndef __AFL_FUZZ_TESTCASE_LEN
    free(buf);
#endif
    return 0;
}
