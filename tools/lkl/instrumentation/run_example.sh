#!/bin/bash
clang -Xclang -ast-dump -fsyntax-only test.c
clang -Xclang -load \
      -Xclang libPluginExample.so \
      -Wall -c test.c
./a.out