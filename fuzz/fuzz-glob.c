#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <glob.h>

#ifdef MUSL_PREFIX
extern int musl_glob(const char *pattern, int flags, int (*errfunc)(const char *, int), glob_t *pglob);
extern void musl_globfree(glob_t *pglob);
#define GLOB musl_glob
#define GLOBFREE musl_globfree
#else
#define GLOB glob
#define GLOBFREE globfree
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

        // First byte is flags
        int flags = GLOB_NOSORT; // Always nosort for speed
        if (buf[0] & 0x01) flags |= GLOB_ERR;
        if (buf[0] & 0x02) flags |= GLOB_MARK;
        if (buf[0] & 0x04) flags |= GLOB_NOCHECK;
        if (buf[0] & 0x08) flags |= GLOB_NOESCAPE;

        // Rest is the pattern
        size_t pattern_len = len - 1;
        char *pattern = malloc(pattern_len + 1);
        if (!pattern) continue;
        memcpy(pattern, buf + 1, pattern_len);
        pattern[pattern_len] = '\0';

        glob_t gl;
        memset(&gl, 0, sizeof(gl));

        // Test glob - use /tmp as a safe directory to glob
        int ret = GLOB(pattern, flags, NULL, &gl);

        if (ret == 0) {
            GLOBFREE(&gl);
        }

        free(pattern);
    }

#ifndef __AFL_FUZZ_TESTCASE_LEN
    free(buf);
#endif
    return 0;
}
