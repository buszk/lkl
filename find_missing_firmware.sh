#!/bin/bash
missing=()
if [ $# -ne 1 ]; then
    echo Usage: $0 [pci/usb]
fi
for f in generated/$1/*; do
    firmware=`timeout 1s tools/lkl/harness/afl-harness $1:fuzz $f 2>&1|grep "Direct firmware load" |grep -v regulatory.db |awk '{print $9}'`
    if [ -z $firmware ]; then
        continue
    fi
    if printf '%s\0' "${missing[@]}" | grep -F -x -z "$firmware" &>/dev/null; then
        continue
    else
        missing+=($firmware)
        echo $firmware
    fi
done
