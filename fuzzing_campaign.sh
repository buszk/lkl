#!/bin/bash
function fuzz {
    harness=~/Workspace/git/lkl/tools/lkl/harness/$1
    target=$2:fuzz
    cores=$3
    input=$4
    secs=86400
    output=campaign-$2
    rm -rf $output
    mkdir $output
    cpu=0
    count=1
    AFL_NO_UI=1 ./afl-fuzz -b $cpu -V $secs -i $input -o $output -t 1000 -M fuzzer$count -- \
        $harness $target @@ &>/dev/null &
    while [ $count -lt $cores ]; do
        cpu=$(($cpu+1))
        count=$(($count+1))
        AFL_NO_UI=1 ./afl-fuzz -b $cpu -V $secs -i $input -o $output -t 1000 -S fuzzer$count -- \
            $harness $target @@ &>/dev/null &
    done
    wait

    total_execs=$(cat $output/fuzzer*/fuzzer_stats |grep execs_done| awk '{print $3}'|awk '{s+=$1} END {print s}')
    echo Fuzzing speed: $1 $target $cores $(($total_execs/$secs))
}

if [ $# -ne 1 ]; then
    echo Usage: $0 [usb/pci]
    exit
fi

cd ../AFLplusplus/
fuzz afl-harness $1 64 ../lkl/generated/$1
cd ../lkl
