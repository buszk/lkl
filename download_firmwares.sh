#!/bin/bash

mkdir -p downloads
cd downloads
if [ ! -d linux-firmware ]; then
    echo "Downloading linux-firmware"
    git clone https://github.com/endlessm/linux-firmware.git
fi
if [ ! -f atmel-firmware_1.3.orig.tar.gz ]; then
    echo "Downloading atmel firmware"
    wget http://archive.ubuntu.com/ubuntu/pool/multiverse/a/atmel-firmware/atmel-firmware_1.3.orig.tar.gz
    tar xvf atmel-firmware_1.3.orig.tar.gz
    cp atmel-firmware-1.3.orig/images.usb/* linux-firmware
    cp atmel-firmware-1.3.orig/images/* linux-firmware
fi
if [ ! -f zd1201-0.14-fw.tar.gz ]; then
    echo "Downloading zd1201.fw"
    wget -O zd1201-0.14-fw.tar.gz 'http://prdownloads.sourceforge.net/linux-lc100020/zd1201-0.14-fw.tar.gz?download'
    tar xvf zd1201-0.14-fw.tar.gz
    cp zd1201-0.14-fw/*.fw  linux-firmware
fi
if [ ! -f isl3886pci ]; then
    echo "Downloading isl3886pci"
    wget -O isl3886pci https://github.com/OpenELEC/wlan-firmware/blob/master/firmware/isl3886pci?raw=true
    cp isl3886pci linux-firmware/
fi
if [ ! -f isl3887usb ]; then
    echo "Downloading isl3887usb"
    wget -O isl3887usb https://github.com/OpenELEC/wlan-firmware/blob/master/firmware/isl3887usb?raw=true
    cp isl3887usb linux-firmware/
fi
if [ ! -f windows_drivers_sr02-2.3.zip ]; then
    echo "Downloading orinoco firmware"
    wget http://web.archive.org/web/20061206062642/http://www.agere.com/mobility/docs/windows_drivers_sr02-2.3.zip
    unzip windows_drivers_sr02-2.3.zip WLAGS51.SYS
    dd if=WLAGS51.SYS of=orinoco_ezusb_fw skip=10312 count=436 bs=16
    cp orinoco_ezusb_fw linux-firmware
fi
if [ ! -f mt7603_e2.bin ]; then
    echo "Downloading mt7603_e2.bin"
    wget -O mt7603_e2.bin https://github.com/openwrt/mt76/blob/master/firmware/mt7603_e2.bin?raw=true
    cp mt7603_e2.bin linux-firmware
fi
cd ..