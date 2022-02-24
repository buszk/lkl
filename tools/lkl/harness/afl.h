inline void __AFL_INIT(void) {
    do {
        static volatile char *_A __attribute__((used));
        _A = (char*) "##SIG_AFL_DEFER_FORKSRV##";
        __attribute__((visibility("default")))
        void _I(void) __asm__("__afl_manual_init");
        _I();
    } while (0);
} 