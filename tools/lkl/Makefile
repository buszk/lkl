# Do not use make's built-in rules
# (this improves performance and avoids hard-to-debug behaviour);
# also do not print "Entering directory..." messages from make
.SUFFIXES:
MAKEFLAGS += -r --no-print-directory
ifneq (${AFL_LLVM_CMPLOG},1)
KCONFIG?=defconfig
else
KCONFIG?=cmplog_defconfig
endif
LDLIBS=-lcapstone

LDFLAGS += -fuse-ld=gold -rdynamic# essential for -fsanitize=fuzzer-no-link
CLANG=/home/buszk/Workspace/git/AFLplusplus/afl-clang-fast
CLANG=clang
CLANG_ARGS= CONFIG_CC_IS_CLANG=y LLVM=1 CC=${CLANG} LD=ld.gold \
  AS=llvm-as AR=llvm-ar NM=llvm-nm STRIP=llvm-strip \
  OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump READELF=llvm-readelf \
  HOSTCC=${CLANG} HOSTCXX=${CLANG}++ HOSTAR=llvm-ar HOSTLD=ld.gold  HOSTAS=llvm-as

# CLANG_ARGS= CC=/home/buszk/Workspace/git/AFLplusplus/afl-gcc \
# 	HOSTCC=gcc

# CLANG_ARGS= CC=gcc

ifneq ($(silent),1)
  ifneq ($(V),1)
	QUIET_AUTOCONF       = @echo '  AUTOCONF '$@;
	Q = @
  endif
endif

PREFIX   := /usr

ifeq (,$(srctree))
  srctree := $(patsubst %/,%,$(dir $(shell pwd)))
  srctree := $(patsubst %/,%,$(dir $(srctree)))
endif
export srctree

-include ../scripts/Makefile.include

# OUTPUT fixup should be *after* include ../scripts/Makefile.include
ifneq ($(OUTPUT),)
  OUTPUT := $(OUTPUT)/tools/lkl/
else
  OUTPUT := $(CURDIR)/
endif
export OUTPUT


all:

conf: $(OUTPUT)Makefile.conf

$(OUTPUT)Makefile.conf: Makefile.autoconf
	$(call QUIET_AUTOCONF, headers)$(MAKE) -f Makefile.autoconf -s

-include $(OUTPUT)Makefile.conf

export CFLAGS += -I$(OUTPUT)/include -Iinclude -Wall -g -O2 -Wextra \
	 -Wno-unused-parameter \
	 -Wno-missing-field-initializers -fno-strict-aliasing

-include Targets

TARGETS := $(progs-y:%=$(OUTPUT)%$(EXESUF))
TARGETS += $(libs-y:%=$(OUTPUT)%$(SOSUF))
all: $(TARGETS)

tests: $(TARGETS)
	harness/tests/tester.sh

# this workaround is for FreeBSD
bin/stat:
ifeq ($(LKL_HOST_CONFIG_BSD),y)
	$(Q)ln -sf `which gnustat` bin/stat
	$(Q)ln -sf `which gsed` bin/sed
else
	$(Q)touch bin/stat
endif

configs:
	cp ../../arch/lkl/configs/defconfig ../../arch/lkl/configs/cmplog_defconfig
	echo "\nCONFIG_CMPLOG=y" >>../../arch/lkl/configs/cmplog_defconfig

instrument:
	make -C instrumentation

# rule to build lkl.o
$(OUTPUT)lib/lkl.o: bin/stat configs instrument
	$(Q)$(MAKE) -C ../.. ARCH=lkl $(CLANG_ARGS) $(KOPT) $(KCONFIG)
	sleep 0.1
	$(MAKE) -C ../.. ARCH=lkl $(CLANG_ARGS) $(KOPT)
# this workaround is for arm32 linker (ld.gold)
	$(Q)export PATH=$(srctree)/tools/lkl/bin/:${PATH} ;\
	$(MAKE) -C ../.. ARCH=lkl $(CLANG_ARGS) $(KOPT) install INSTALL_PATH=$(OUTPUT)

# rules to link libs
$(OUTPUT)%$(SOSUF): LDFLAGS += -shared
$(OUTPUT)%$(SOSUF): $(OUTPUT)%-in.o $(OUTPUT)liblkl.a
	$(QUIET_LINK)$(CC) $(LDFLAGS) $(LDFLAGS_$*-y) -o $@ $^ $(LDLIBS) $(LDLIBS_$*-y)

# liblkl is special
$(OUTPUT)liblkl$(SOSUF): $(OUTPUT)%-in.o $(OUTPUT)lib/lkl.o
$(OUTPUT)liblkl.a: $(OUTPUT)lib/liblkl-in.o $(OUTPUT)lib/lkl.o
	$(QUIET_AR)$(AR) -rc $@ $^

# rule to link programs
$(OUTPUT)%$(EXESUF): $(OUTPUT)%-in.o $(OUTPUT)liblkl.a
	$(QUIET_LINK)$(CC) $(LDFLAGS) $(LDFLAGS_$*-y) -o $@ $^ $(LDLIBS) $(LDLIBS_$*-y)

# rule to build objects
$(OUTPUT)%-in.o: $(OUTPUT)lib/lkl.o FORCE
	$(Q)$(MAKE) -f $(srctree)/tools/build/Makefile.build dir=$(patsubst %/,%,$(dir $*)) obj=$(notdir $*)


$(OUTPUT)cpfromfs$(EXESUF): cptofs$(EXESUF)
	$(Q)if ! [ -e $@ ]; then ln -s $< $@; fi

clean:
	$(call QUIET_CLEAN, objects)find $(OUTPUT) -name '*.o' -delete -o -name '\.*.cmd'\
	 -delete -o -name '\.*.d' -delete
	$(call QUIET_CLEAN, headers)$(RM) -r $(OUTPUT)/include/lkl/
	$(call QUIET_CLEAN, liblkl.a)$(RM) $(OUTPUT)/liblkl.a
	$(call QUIET_CLEAN, targets)$(RM) $(TARGETS) bin/stat

clean-conf: clean
	$(call QUIET_CLEAN, Makefile.conf)$(RM) $(OUTPUT)/Makefile.conf

headers_install: $(TARGETS)
	$(call QUIET_INSTALL, headers) \
	    install -d $(DESTDIR)$(PREFIX)/include ; \
	    install -m 644 include/lkl.h include/lkl_host.h $(OUTPUT)include/lkl_autoconf.h \
	      include/lkl_config.h $(DESTDIR)$(PREFIX)/include ; \
	    cp -r $(OUTPUT)include/lkl $(DESTDIR)$(PREFIX)/include

libraries_install: $(libs-y:%=$(OUTPUT)%$(SOSUF)) $(OUTPUT)liblkl.a
	$(call QUIET_INSTALL, libraries) \
	    install -d $(DESTDIR)$(PREFIX)/lib ; \
	    install -m 644 $^ $(DESTDIR)$(PREFIX)/lib

programs_install: $(progs-y:%=$(OUTPUT)%$(EXESUF))
	$(call QUIET_INSTALL, programs) \
	    install -d $(DESTDIR)$(PREFIX)/bin ; \
	    install -m 755 $^ $(DESTDIR)$(PREFIX)/bin

install: headers_install libraries_install programs_install


run-tests:
	./tests/run.py $(tests)

FORCE: ;
.PHONY: all clean FORCE run-tests
.PHONY: headers_install libraries_install programs_install install
.NOTPARALLEL : lib/lkl.o
.SECONDARY:

