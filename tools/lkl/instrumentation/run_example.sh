#!/bin/bash
clang -Xclang -ast-dump -fsyntax-only test.c
clang -Xclang -load \
      -Xclang libDriverModifier.so \
      -Wall -c test.c
./a.out