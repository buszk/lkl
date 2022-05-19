#!/usr/bin/env python3
import os
import glob
import shutil
import subprocess
import argparse
import itertools
from pathlib import Path
from statistics import mean

agamotto_alias = {
    '8139cp': 'rtl8139',
    'atlantic': 'aqc100',
    'stmmac_pci': 'quark',
    'mwifiex_usb': 'mwifiex',
    'rsi_usb': 'rsi',
}

targets = ["ath9k", "ath10k_pci", "rtwpci", "8139cp", "atlantic", "stmmac_pci", "snic"]
targets += ["ar5523", "mwifiex_usb", "rsi_usb"]
harnesses = ["afl-forkserver-harness", "afl-delayed-forkserver-harness", "afl-harness"]

data_dir = Path(f'/data/{os.environ.get("USER")}/lkl')
# outputs_dir = Path(f'{os.environ.get("HOME")}/Workspace/git/AFLplusplus/outputs')
outputs_dir = Path.cwd() / ".." / "AFLplusplus" / "outputs"

parser = argparse.ArgumentParser()
parser.add_argument("--cleanup", default=False, action="store_true")
args = parser.parse_args()

max_iter = 3
count = 0
while count < max_iter:
    count += 1
    if (data_dir / str(count)).exists():
        if args.cleanup:
            print("Clean up exisitng data")
            shutil.rmtree(data_dir / str(count))
        else:
            print(f"exisitng data in {data_dir}")
            continue

    subprocess.check_call('./stats.sh')

    for subdir in outputs_dir.iterdir():
        if subdir.is_dir() and subdir.name.endswith('harness'):
            shutil.move(subdir, data_dir / str(count) / subdir.name)

agamotto_map = {}
USER = os.environ.get('USER')
for t in targets:
    alias = t
    if alias in agamotto_alias:
        alias = agamotto_alias[alias]
    time = 0
    execs = 0
    for f in glob.glob(f'/data/{USER}/agamotto/{alias}-*/out/fuzzer*/fuzzer_stats'):
        inf = open(f)
        for line in inf:
            if 'start_time' in line:
                time -= int(line.split(':')[1][1:])
            elif 'last_update' in line:
                time += int(line.split(':')[1][1:])
            elif 'execs_done' in line:
                execs += int(line.split(':')[1][1:])
    # print(f"{t}: {execs/time}")
    if time == 0:
        for f in glob.glob(f'/data/{USER}/agamotto-usb/{alias}-*/{alias}-*.log'):
            inf = open(f)
            for line in reversed(inf.readlines()):
                if 'executed' in line:
                    execs += int(line.split(' ')[5][:-1])
                    time += 3600*16
                    break
    agamotto_map[t] = execs/time if time else 1

result_map = {}
count = 0
while count < max_iter:
    count += 1
    outputs_dir = data_dir / str(count)
    for subdir in outputs_dir.iterdir():
        stats_file = subdir / "default" / "fuzzer_stats"
        target = subdir.name.split('-')[1]
        harness = subdir.name.replace(f'output-{target}-', '')
        if stats_file .exists():
            with open(stats_file) as f:
                for line in f:
                    if 'execs_per_sec' in line:
                        execs_per_sec = float(line.split(": ")[1][:-1])
                        # print(target, harness, execs_per_sec)
                        if count == 1:
                            result_map[(target, harness)] = []
                        result_map[(target, harness)].append(execs_per_sec)
                        break


print("Target & Agamotto & Forkserver & Delayed fork & Our fuzzer & Speed up \\\\\\hline")
for t in targets:
    print(t, end='')
    print(' & ', end='')
    print(round(agamotto_map[t], 1), end='')
    for h in harnesses:
        print(' & ', end='')
        if (t, h) in result_map:
            avg = round(mean(result_map[(t, h)]), 1)
        else:
            avg = 'x'
        print(avg, end='')
    print(' & ', end='')
    print(round(avg/agamotto_map[t]) if avg != 'x' else 'x', end='')
    print('\\\\\\hline')
