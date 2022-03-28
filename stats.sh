#!/bin/bash
targets=(
    'ath9k'
    'ath10k_pci'
    'rtwpci'
    '8139cp'
    'atlantic'
    'stmmac_pci'
    'snic'
)
harnesses=(
    'afl-harness'
    'afl-forkserver-harness'
)

function fuzz {
    harness=~/Workspace/git/lkl/tools/lkl/harness/$1
    target=$2
    secs=3600
    mkdir -p outputs
    output=outputs/output-$target-$1
    rm -rf $output
    cpu_id=$3
    echo $cpu_id
    AFL_NO_UI=1 ./afl-fuzz -b $cpu_id -V $secs -i input -o $output -t 1000 -- \
        $harness $target @@ &>/dev/null
    echo Fuzzing speed: $target $1
    grep execs_per_sec $output/default/fuzzer_stats
}

cpu=0
cd ../AFLplusplus/
for t in ${targets[@]}; do
    for h in ${harnesses[@]}; do
        fuzz $h $t $cpu &
        cpu=$(($cpu+2))
    done
done

wait
cd ../lkl
