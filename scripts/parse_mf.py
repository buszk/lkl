#!/usr/bin/env python3


import os
import re
import sys
import glob


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
    if not os.path.exists(cf):
        return {}

    with open(cf, 'r') as fd:
        data = fd.read()
    conf = None
    confmap = {}
    for line in data.split('\n'):
        if line.startswith("config"):
            conf = line.split()[1]
        elif line.strip().startswith("depends on"):
            confmap[conf] = line.split()[2:]
    #print(confmap)
    return confmap


wordlist = [
        "pci",
        ]
match = re.compile("|".join(wordlist))
def interesting(key, dname):
    if dname in key:
        return True

    if match.match(key):
        return True

    return False


def scrape_dir(path):
    for mf in glob.glob(os.path.join(path, "**", "Makefile"), recursive=True):
        objmap = build_objmap(mf)
        confmap = build_confmap(os.path.join(os.path.dirname(mf), "Kconfig"))
        for obj in objmap:
            if interesting(obj, os.path.basename(os.path.dirname(mf))):
                conf = objmap[obj][0]
                while not conf.startswith("$("):
                    conf = objmap[conf][0]
                conf = conf[2+len("CONFIG_"):-1]
                print(mf)
                print(conf)
                if conf in confmap:
                    print(confmap[conf])

if __name__== '__main__':
    testfile = sys.argv[1]
    #build_objmap(testfile)
    #build_confmap(testfile)
    scrape_dir(testfile)
