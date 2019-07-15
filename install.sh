#!/bin/bash

CWD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Exit if something fails
set -e

# AUR_install <pkgname> <dirname> <sed argument>
function AUR_install {
	curl https://aur.archlinux.org/packages/${1:0:2}/$1/$1.tar.gz | tar xz
	cd $2
	if [ $# -eq 3 ]; then
		sed -i "$3" PKGBUILD
	fi
	makepkg -si --noconfirm -A --asroot
	cd ..
}

# Bugfix: EIBd bring eibloadresult.h per default nicht mit
cp ./eibloadresult.h /usr/include

# Hostname
echo "saallicht" > /etc/hostname

# Passwort
echo "Root Passwort eingeben:"
passwd

pacman -Sy base-devel --noconfirm

cd /tmp

# pthsem, dependency von EIBd installieren
AUR_install pthsem pthsem

# EIBd mit Support für den TUL tpuarts adapter bauen
AUR_install eibd bcusdk "/.\/configure \\\/a \	--enable-tpuarts \\\\"

# HIDapi (dependency von dmxknx mit dem Digital Enlightenment / FX5) bauen
AUR_install hidapi hidapi

echo -e 'EIBD_OPTS=\" -i tpuarts:/dev/ttyACM0 \"' > /etc/conf.d/eibd.conf

mkdir -p /opt/saallicht
cp -R $CWD/* /opt/saallicht
cd /opt/saallicht
make
cp services/*.service /etc/systemd/system
systemctl enable saallicht_eibd
systemctl enable saallicht_gateway

# Make filesystem readonly
#FSTAB_VAR="\ntmpfs   /var/log    tmpfs   nodev,nosuid    0   0\ntmpfs   /var/tmp    tmpfs   nodev,nosuid    0   0"
#CMDLINE="ipv6.disable=1 avoid_safe_mode=1 selinux=0 plymouth.enable=0 smsc95xx.turbo_mode=N dwc_otg.lpm_enable=0 console=ttyAMA0,115200 kgdboc=ttyAMA0,115200 console=tty1 root=/dev/mmcblk0p5 ro rootfstype=ext4 elevator=noop rootwait"
#echo -e $CMDLINE > /boot/cmdline.txt
#rm /etc/resolv.conf
#ln -s /tmp/resolv.conf /etc/resolv.conf
#echo -e $FSTAB_VAR >> /etc/fstab
#systemctl disable systemd-readahead-collect
#systemctl disable systemd-random-seed
#systemctl disable ntpd
