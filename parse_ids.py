#!/usr/bin/env python3

import os
import random

conf = {
    "pci": "pci_ids.log",
    "usb": "usb_ids.log",
}
for bus, log in conf.items():
    ids = []
    with open(log, 'r') as f:
        for line in f.readlines():
            line = line.replace('\n', '')
            sp = line.split(' ')
            idstr = sp[0]
            driver_name = sp[1]
            sp = idstr.split(':')
            if len(sp) != 3 or sp[0] != bus:
                continue
            ids.append([int(sp[1], 16), int(sp[2], 16)])
    # os.makedirs("generated", exist_ok=True)
    os.makedirs(f"generated/{bus}", exist_ok=True)
    for id in ids:
        with open(f"generated/{bus}/{id[0]:0x}_{id[1]:0x}", 'wb') as f:
            f.write(id[0].to_bytes(2, 'little'))
            f.write(id[1].to_bytes(2, 'little'))
            f.write(bytearray(random.getrandbits(8) for _ in range(4096)))
