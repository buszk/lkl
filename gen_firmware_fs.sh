#!/bin/bash
sudo pwd
format=ext4
file=firmware
dd if=/dev/zero of=$file.$format bs=1024 count=1024000
yes | mkfs.$format $file.$format
sudo mkdir -p /mnt/firmware
sudo mount -o rw $file.$format /mnt/firmware

# rsync -ah --progress $HOME/DrifuzzRepo/linux-firmware/* /mnt/firmware
sudo cp -r $HOME/DrifuzzRepo/linux-firmware/* /mnt/firmware
sudo cp /usr/lib/firmware/regulatory.db /mnt/firmware
echo test |sudo tee /mnt/firmware/test
sudo chown -R $USER:$USER /mnt/firmware

sudo umount /mnt/firmware