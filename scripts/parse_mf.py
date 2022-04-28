#!/usr/bin/env python3


from mmap import mmap, ACCESS_READ
import os
import re
import sys
import glob
from os.path import join, exists, dirname, basename, isdir


def build_objmap(mf):
    with open(mf, 'r') as fd:
        data = fd.read()
    lines = data.split('\n')
    i = 0
    prep_lines = []
    while i < len(lines):
        line = lines[i]
        i += 1
        if line == "" or line[0] == "#":
            continue
        while line[-1] == '\\':
            if i >= len(lines):
                break
            line = line[:-1] + lines[i]
            i += 1
        prep_lines.append(line)

    objmap = {}
    for line in prep_lines:
        tup = line.split()
        if '-' not in tup[0]:
            continue
        nctup = tup[0].split('-')
        if len(nctup) > 2:
            continue
        nam, conf = nctup
        op = tup[1]
        targets = tup[2:]
        if (conf == "objs" or conf == 'y'):
            objmap.setdefault(nam+'.o', []).extend(targets)
        elif (conf.startswith("$(")):
            objmap.setdefault(conf, []).extend(targets)
    #print(objmap)

    confmap = {}
    for k in objmap:
        for target in objmap[k]:
            if target.startswith('-'):
                continue
            confmap.setdefault(target, []).append(k)
    #print(confmap)
    return confmap

def build_confmap(cf):
    if not exists(cf):
        return {}

    with open(cf, 'r') as fd:
        data = fd.read()
    conf = None
    confmap = {}
    for line in data.split('\n'):
        if line.startswith("config"):
            conf = line.split()[1]
            confmap[conf] = []
        elif line.strip().startswith("depends on"):
            confmap[conf] = line.split()[2:]
    #print(confmap)
    return confmap


def files_have_keyword(dir):
    files = []
    for f in glob.glob(join(dir, "**")):
        if isdir(f):
            continue
        with open(f, 'rb', 0) as file, mmap(file.fileno(), 0, access=ACCESS_READ) as s:
            if s.find(b'USB_DEVICE') != -1:
                files.append(basename(f))
            elif s .find(b'PCI_DEVICE') != -1:
                files.append(basename(f))
    return files


def scrape_dir(path):
    config_match = re.compile(r"\$\((CONFIG_[A-Za-z0-9_]*?)\)")
    allconfigs = set()
    alldeps = set()
    for mf in glob.glob(join(path, "**", "Makefile"), recursive=True):
        confmap = build_confmap(join(dirname(mf), "Kconfig"))
        files = files_have_keyword(dirname(mf))
        if not files:
            continue

        # Proceed only when any file in driver directory contains keywords
        instrumented = False
        configs = set()
        with open(mf, 'r') as file:
            for line in file.readlines():
                [configs.add(conf) for conf in config_match.findall(line)]
                instrumented = instrumented or "AFLCOV" in line
        configs = configs.intersection([f"CONFIG_{x}" for x in confmap.keys()])
        for conf in configs:
            alldeps.update(confmap[conf.replace("CONFIG_", "")] if conf.replace("CONFIG_", "") in confmap else set())
        if configs:
            print(mf)
            print(configs)
            allconfigs.update(configs)
        # instrument AFLCOV to driver Makefile
        if not instrumented:
            with open(mf, 'r') as contents:
                save = contents.read()
                line1 = save.split('\n')[0]
                save = save[len(line1)+1:]
            with open(mf, 'w') as contents:
                contents.write(f"{line1}\nAFLCOV := y\n")
            with open(mf, 'a') as contents:
                contents.write(save)
                
    # Print results
    print()
    [print(f"deps on {conf}") for conf in alldeps]
    print()
    [print(f"{conf}=y") for conf in allconfigs]


if __name__== '__main__':
    testfile = sys.argv[1]
    #build_objmap(testfile)
    #build_confmap(testfile)
    scrape_dir(testfile)
