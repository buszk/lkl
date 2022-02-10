#!/bin/bash

yes "" | make -C tools/lkl -j32 2>&1|tee compile_mod.log
