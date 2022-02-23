#!/bin/bash

DIR=$(readlink -f "$(dirname "$0")")

COUNTER=0
if ${DIR}/kasan-test-soob 2>&1 | grep -q "BUG: KASAN: slab-out-of-bounds"; then
    echo "KASAN soob okay"
else
    let COUNTER=COUNTER+1
    echo "KASAN soob test failed"
fi

if ${DIR}/kasan-test-uaf 2>&1 | grep -q "BUG: KASAN: use-after-free"; then
    echo "KASAN uaf okay"
else
    let COUNTER=COUNTER+1
    echo "KASAN uaf test failed"
fi

if [ ${COUNTER} -ne 0 ]; then
    echo "KASAN test failed"
    exit 1
fi