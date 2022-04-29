#ifndef _FUZZ_H
#define  _FUZZ_H
#include <asm/setjmp.h>
extern int input_end;
extern int jmp_buf_valid;
extern struct jmp_buf_data jmp_buf;

struct jmp_buf_data* push_jmp_buf(void);
struct jmp_buf_data* pop_jmp_buf(void);
struct jmp_buf_data* try_pop_jmp_buf(void);

#define nowait_wait_for_completion_timeout(a, b) wait_for_completion_timeout(a, 0)
#define nowait_mod_timer(a, b) mod_timer(a, jiffies)
#define nowait_usleep_range(a, b)
#define nowait_msleep(a)
#define nowait_ssleep(a)
#define nowait_schedule_timeout_uninterruptible(a)

#endif