#ifndef _FUZZ_H
#define  _FUZZ_H
#include <asm/setjmp.h>
extern int input_end;
extern int jmp_buf_valid;
extern struct jmp_buf_data jmp_buf;

struct jmp_buf_data* push_jmp_buf(void);
struct jmp_buf_data* pop_jmp_buf(void);
struct jmp_buf_data* try_pop_jmp_buf(void);

#endif