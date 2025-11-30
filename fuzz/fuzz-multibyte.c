#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>

#ifdef MUSL_PREFIX
extern size_t musl_mbstowcs(wchar_t *dest, const char *src, size_t n);
extern size_t musl_wcstombs(char *dest, const wchar_t *src, size_t n);
extern size_t musl_mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);
extern int musl_mblen(const char *s, size_t n);
#define MBSTOWCS musl_mbstowcs
#define WCSTOMBS musl_wcstombs
#define MBRTOWC musl_mbrtowc
#define MBLEN musl_mblen
#else
#define MBSTOWCS mbstowcs
#define WCSTOMBS wcstombs
#define MBRTOWC mbrtowc
#define MBLEN mblen
#endif

#ifdef __AFL_FUZZ_TESTCASE_LEN
__AFL_FUZZ_INIT();
#endif

int main(int argc, char **argv) {
    setlocale(LC_ALL, "C.UTF-8");

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

        // Null-terminate
        char *input = malloc(len + 1);
        if (!input) continue;
        memcpy(input, buf, len);
        input[len] = '\0';

        // Test mbstowcs - multibyte to wide string
        wchar_t wbuf[1024];
        size_t wlen = MBSTOWCS(wbuf, input, 1023);

        if (wlen != (size_t)-1 && wlen > 0) {
            // Test wcstombs - wide to multibyte
            char mbuf[4096];
            WCSTOMBS(mbuf, wbuf, sizeof(mbuf) - 1);
        }

        // Test mbrtowc
        wchar_t wc;
        mbstate_t state;
        memset(&state, 0, sizeof(state));

        const char *p = input;
        size_t remaining = len;
        while (remaining > 0) {
            size_t r = MBRTOWC(&wc, p, remaining, &state);
            if (r == 0 || r == (size_t)-1 || r == (size_t)-2) break;
            p += r;
            remaining -= r;
        }

        // Test mblen
        MBLEN(NULL, 0); // Reset state
        for (size_t i = 0; i < len; ) {
            int r = MBLEN(input + i, len - i);
            if (r <= 0) break;
            i += r;
        }

        free(input);
    }

#ifndef __AFL_FUZZ_TESTCASE_LEN
    free(buf);
#endif
    return 0;
}
