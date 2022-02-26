#!/bin/bash
echo "reproduce log:" > reproduce.log
for f in ../AFLplusplus/output/default/crashes/*; do
    echo $f >> reproduce.log
    (/bin/sh -c "tools/lkl/harness/afl-harness $f") >> reproduce.log 2>&1
    code=$?
    if [ $code -gt 1 ];then
        echo "non-zero exit code: $code" >> reproduce.log
    fi
done