#!/bin/bash
echo "reproduce log:" > reproduce.log
for f in ../AFLplusplus/output/default/queue/*; do
    echo $f >> reproduce.log
    tools/lkl/harness/afl-harness $f >> reproduce.log
    code=$?
    if [ $code -gt 1 ];then
        echo "non-zero exit code: $code" >> reproduce.log
    fi
done