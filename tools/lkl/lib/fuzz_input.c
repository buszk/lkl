#include <lkl.h>
#include <stdlib.h>
#include <string.h>
#include "fuzz_input.h"


uint8_t* buffer;
size_t size = 0;
size_t used = 0;

#define GET_FUNC(ty, nm) \
    ty get_##nm (void) { \
        if (used + sizeof( ty ) > size) \
            exit(1); \
        ty res = *( ty *)(buffer+used); \
        used += sizeof( ty ); \
        return res; \
    }

GET_FUNC(uint8_t, byte)
GET_FUNC(uint16_t, word)
GET_FUNC(uint32_t, dword)
GET_FUNC(uint64_t, qword)

void get_size(size_t s, void* b) {
    if (used + s > size)
        exit(1);
    memcpy(b, buffer + used, s);
    used += s;
}

void lkl_set_fuzz_input(void* inp, size_t s) {
    buffer = inp;
    size = s;
}