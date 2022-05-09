#!/bin/bash

REPEAT=0
while getopts "r" o; do
    case "${o}" in
        r)
			REPEAT=1
            shift
            ;;
        *)
            ;;
    esac
done

if [ $# -ne 1 ];then
    echo $0 "[pci/usb]"
    exit
fi

randfile=
if [ $REPEAT -eq 1 ];then
    randfile=$(cat randfile.log)
else
    randfile=$(find generated/usb/ -type f |shuf -n 1)
    echo -n $randfile >randfile.log
fi
tools/lkl/harness/afl-harness $1:fuzz $randfile
echo $randfile