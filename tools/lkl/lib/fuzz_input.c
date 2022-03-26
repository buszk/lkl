#include <lkl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include "memwatcher.h"
#include "fuzz_input.h"


uint8_t* buffer;
size_t size = 0;
size_t used = 0;
int overflow = 0;

static int fuzz_mode = MODE_DEFAULT;

static int coverage = 0;
int __afl_selective_coverage = 1;
static void afl_coverage_off(void) {
    if (coverage) {
        coverage = 0;
        __attribute__((visibility("default")))
        void _OFF(void) __asm__("__afl_coverage_off");
        _OFF();
        fprintf(stderr, "Disable coverage collection because of limited input\n");
    }
}

jmp_buf jmp_env;
int jmp_env_set;
static void afl_coverage_on(void) {
    if (!coverage) {
        coverage = 1;
        __attribute__((visibility("default")))
        void _ON(void) __asm__("__afl_coverage_on");
        _ON();
    }
}

static inline void input_end(void) {
    fprintf(stderr, "Too short\n");
    switch (fuzz_mode)
    {
    case MODE_DEFAULT:
    case MODE_PERSISTENT:
        afl_coverage_off();
        /* if not inside segfault handler */
        if (!_in_cb())
            lkl_set_input_end(1);
        overflow = 1;
        // if (jmp_env_set)
        //     longjmp(jmp_env, 41);
        // lkl_dump_stack();

        break;
    case MODE_FORKSERVER:
        exit(1);
        break;
    default:
        break;
    }
}

#define GET_FUNC(ty, nm) \
    ty get_##nm (void) { \
        if (used + sizeof( ty ) > size) { \
            input_end(); \
            return rand(); \
        } \
        ty res = *( ty *)(buffer+used); \
        used += sizeof( ty ); \
        return res; \
    }

GET_FUNC(uint8_t, byte)
GET_FUNC(uint16_t, word)
GET_FUNC(uint32_t, dword)
GET_FUNC(uint64_t, qword)

void get_size(size_t s, void* b) {
    if (used + s > size) {
        afl_coverage_off();
        return;
    }
    memcpy(b, buffer + used, s);
    used += s;
}

void lkl_set_fuzz_input(void* inp, size_t s) {
    overflow = 0;
    _memwatcher_reset();
    afl_coverage_on();
    buffer = inp;
    size = s;
    used = 0;
    lkl_set_input_end(0);
}

void set_fuzz_mode(int mode) {
    fuzz_mode = mode;
}