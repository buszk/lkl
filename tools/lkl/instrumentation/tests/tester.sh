#!/bin/bash

DIR=$(dirname $0)
RC=0
for test in $DIR/match_*.c; do
    clang -Xclang -load \
      -Xclang $DIR/../libDriverModifier.so \
      -Wall -c $test &>/dev/null
    if grep setjmp $test.mod &>/dev/null; then
        echo "Okay   ...  $test"
    else
        echo "Failed ...  $test"
        RC=1
    fi
    # rm $test.mod
done

for test in $DIR/no_match_*.c; do
    clang -Xclang -load \
      -Xclang $DIR/../libDriverModifier.so \
      -Wall -c $test &>/dev/null
    if grep setjmp $test.mod &>/dev/null; then
        echo "Failed ...  $test"
        RC=1
    else
        echo "Okay   ...  $test"
    fi
done

exit $RC