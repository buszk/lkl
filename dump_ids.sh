#!/bin/bash

# Dump pci id
tools/lkl/harness/afl-harness pci:ff:ff:1 random 2>&1|tee log
grep  ^pci: log |egrep -v  LKL\|cavium_rng\|nvme >pci_ids.log

# Dump usb id
tools/lkl/harness/afl-harness usb:1:1 random 2>&1|tee log
grep ^usb: log >usb_ids.log