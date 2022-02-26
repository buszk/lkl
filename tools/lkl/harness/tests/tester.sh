#!/bin/bash

DIR=$(readlink -f "$(dirname "$0")")

FAILED=0
function test_kasan() {
    COUNTER=0
    for i in $(seq 20);do
        /bin/sh -c ${DIR}/kasan-test-$1 >$1.log 2>&1
        if [ $? -eq 0 ];then
            let COUNTER=COUNTER+1
            break
        fi
        if grep -q "$2" $1.log; then
            :
        else
            let COUNTER=COUNTER+1
            break
        fi
    done
    # rm $1.log

    if [ ${COUNTER} -eq 0 ]; then
        echo "ok    ....    $1"
    else
        echo "fail  ....    $1"
        let FAILED=FAILED+1
    fi
}
test_kasan soob "BUG: KASAN: slab-out-of-bounds"
test_kasan uaf "BUG: KASAN: use-after-free"

if [ ${FAILED} -eq 0 ]; then
    echo "KASAN succeed"
else
    echo "KASAN failed"
fi