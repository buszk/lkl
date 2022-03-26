#!/bin/bash

if [ -z $2 ];then
    echo "$0 <target> <crashes/hangs/queue>"
    exit
fi

echo "reproduce log:" > reproduce.log
for f in ../AFLplusplus/output/default/$2/*; do
    echo $f >> reproduce.log
    (/bin/sh -c "time tools/lkl/harness/afl-harness $1 $f") >> reproduce.log 2>&1
    code=$?
    if [ $code -gt 1 ];then
        echo "non-zero exit code: $code" >> reproduce.log
    fi
done
