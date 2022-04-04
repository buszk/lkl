# Userspace Linux Device Driver Testing

## Introduction
This project intends test Linux device drivers in the userspace. Unlike full-system emulation approaches like [Agamotto](https://github.com/securesystemslab/agamotto) and [Drifuzz](https://github.com/buszk/Drifuzz), we emulate devices in userspace with Linux Kernel Library ([LKL](../Documentation/lkl.txt)). Running drivers in userspace, we are able to achieve high execution speed, (x100 more than full-system approaches) and leverage cutting-edge userspace optimization provided in [AFLplusplus](https://github.com/AFLplusplus/AFLplusplus).

## Install
Most advanced optimizations provided by AFLplusplus require LLVM. We have LLVM 11 installed in our system. Make sure `libclang-11-dev` is installed to compile our Clang plugin.
```
# Install AFLplusplus
git clone https://github.com/AFLplusplus/AFLplusplus
cd AFLplusplus
git checkout 4.00c
make
```

```
# Install customized lkl
git clone https://github.com/buszk/lkl
cd lkl
./compile.sh

# KASAN test
tools/lkl/harness/tests/tester.sh

# Run harness
tools/lkl/harness/afl-harness [target] [input]
```

## Input-to-state
```
cd lkl

# Compile cmplog executable
export AFL_LLVM_CMPLOG=1
make clean
./compile.sh
cp tools/lkl/harness/afl-harness tools/lkl/harness/afl-harness.cmplog
unset AFL_LLVM_CMPLOG

# Compile regular executable
make clean
./compile.sh
```

## Fuzzing
Currently supported targets are:
ath9k, ath10k_pci, rtwpci, 8139cp, atlantic, stmmac_pci and snic

```
cd AFLplusplus
# Regular fuzzing
./afl-fuzz -i input -o output -- \
        ../lkl/tools/lkl/harness/afl-harness [target] @@

# input-to-state fuzzing
./afl-fuzz -i input -o output \
        -c ../lkl/tools/lkl/harness/afl-harness.cmplog -- \
        ../lkl/tools/lkl/harness/afl-harness [target] @@
```

## Development
After change, the following commands need to run to build src.
```
make clean      # if kernel Makefile changed
./clean.sh      # if kernel file changed
./compile.sh    # if tools/lkl file changed
```