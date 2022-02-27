static inline void __AFL_INIT(void) {
    do {
        static volatile char *_A __attribute__((used));
        _A = (char*) "##SIG_AFL_DEFER_FORKSRV##";
        __attribute__((visibility("default")))
        void _I(void) __asm__("__afl_manual_init");
        _I();
    } while (0);
} 

// int __afl_persistent_loop(unsigned int max_cnt);
static inline int  __AFL_LOOP(int _A) {
      static volatile char *_B __attribute__((used));
       _B = (char*) "##SIG_AFL_PERSISTENT##";
      __attribute__((visibility("default")))
      int _L(unsigned int) __asm__("__afl_persistent_loop");
      int ret = _L(_A);
      fprintf(stderr, "%s return %d\n", __func__, ret);
      return ret;
}
