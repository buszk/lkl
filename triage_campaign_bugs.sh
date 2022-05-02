#!/bin/bash

if [ $# -ne 1 ]; then
    echo Usage: $0 [pci/usb]
fi
log=$1_bug.log
echo "bug log:" > $log
for f in ../AFLplusplus/campaign-$1/fuzzer*/crashes/*; do
    echo $f >>  $log
    (/bin/sh -c "timeout 2s tools/lkl/harness/afl-harness $1:fuzz $f") >/tmp/reproduce.log 2>&1
    awk '/BUG:/,/addr2line/' /tmp/reproduce.log >>$log
    awk '/Access Instr loc:/,/addr2line/' /tmp/reproduce.log >>$log
done
