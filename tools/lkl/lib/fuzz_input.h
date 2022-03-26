
#include <sys/types.h>
#include <stdint.h>

uint8_t get_byte(void);
uint16_t get_word(void);
uint32_t get_dword(void);
uint64_t get_qword(void);
void get_size(size_t, void*);

extern int overflow;