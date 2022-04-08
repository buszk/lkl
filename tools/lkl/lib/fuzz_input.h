
#include <sys/types.h>
#include <stdint.h>

uint8_t get_byte(void);
uint16_t get_word(void);
uint32_t get_dword(void);
uint64_t get_qword(void);
uint8_t get_control(void);
void get_size(void*, size_t);

extern int overflow;