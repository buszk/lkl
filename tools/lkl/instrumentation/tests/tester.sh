#!/bin/bash

DIR=$(dirname $0)
RC=0
for test in $DIR/match_*.c; do
    clang -Xclang -load \
      -Xclang $DIR/../libPluginExample.so \
      -Wall -c $test >clog 2>&1
    if grep setjmp clog &>/dev/null; then
        echo "Okay   ...  $test"
    else
        echo "Failed ...  $test"
        RC=1
    fi
    rm clog
    # rm $test.mod
done

for test in $DIR/no_match_*.c; do
    clang -Xclang -load \
      -Xclang $DIR/../libPluginExample.so \
      -Wall -c $test >clog 2>&1
    if grep setjmp clog &>/dev/null; then
        echo "Failed ...  $test"
        RC=1
    else
        echo "Okay   ...  $test"
    fi
    rm clog
    rm $test.mod
done

exit $RC