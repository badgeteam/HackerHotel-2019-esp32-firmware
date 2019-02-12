# Badge team ESP32 Firmware

This repository contains the firmware for the SHA2017 badge, the Hackerhotel badge and the Disobey badge.
 
# Resources

* [Project documentation](https://wiki.badge.team)
* [Documentation](https://wiki.badge.team/Firmware)
* [Firmware](https://github.com/badgeteam/ESP32-Firmware)
* [Changelog](CHANGELOG.md)

Copyright (C) 2017-2019 Badge team.
Using [Espressif Audio Development Framework](https://github.com/espressif/esp-adf) Copyright (C) 2018 Espressif Systems.
Based on template application for [Espressif IoT Development Framework (ESP-IDF)](https://github.com/espressif/esp-idf).
Copyright (C) 2016 Espressif Systems, licensed under the Apache License 2.0 as described in the file LICENSE.

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/a61bf7ca8c6040e78382af2741a67d04)](https://www.codacy.com/app/Badgeteam/ESP32-Firmware?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=badgeteam/ESP32-Firmware&amp;utm_campaign=Badge.team)
[![Build Status](https://travis-ci.org/badgeteam/ESP32-Firmware.svg?branch=master)](https://travis-ci.org/badgeteam/ESP32-Firmware)

# Build instructions
To build the basic firmware:
 * make menuconfig
 * make

## Debian prerequisites

```
sudo apt-get install libncurses5-dev flex bison gperf python-serial libffi-dev libsdl2-dev libmbedtls-dev perl
```

## Compiling and flashing

```
git submodule update --init --recursive
source set_env.sh
make defconfig
make -j5
```

## Optional configuration

```
make menuconfig
```

# Interacting via serial

```
make monitor
```

# Building on OSX

First you have to replace the linux toolchain:
* download the [mac toolchain](https://dl.espressif.com/dl/xtensa-esp32-elf-osx-1.22.0-61-gab8375a-5.2.0.tar.gz) from [this page](https://dl.espressif.com/doc/esp-idf/latest/get-started/macos-setup.html)
* rename the linux toolchain: `mv xtensa-esp32-elf xtensa-esp32-elf-linux`
* untar the mac toolchain: ` tar zxf ~/Downloads/xtensa-esp32-elf-osx-1.22.0-61-gab8375a-5.2.0.tar.gz`

Then install the `CP2102` [usbserial driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers)

In the `sdkconfig` file change this line:

    CONFIG_ESPTOOLPY_PORT="/dev/tty.SLAB_USBtoUART"

# MicroPython

```
import badge
badge.eink_init()
badge.display_picture(0,-1)
import ugfx
ugfx.init()
ugfx.demo("HACKING")
ugfx.clear(badge.BLACK)
ugfx.thickline(1,1,100,100,badge.WHITE,10,5)
ugfx.box(30,30,50,50,badge.WHITE)
ugfx.string(150,25,"STILL","Roboto_BlackItalic24",badge.WHITE)
ugfx.string(130,50,"Hacking","PermanentMarker22",badge.WHITE)
len = ugfx.get_string_width("Hacking","PermanentMarker22")
ugfx.line(130, 72, 144 + len, 72, badge.WHITE)
ugfx.line(140 + len, 52, 140 + len, 70, badge.WHITE)
ugfx.string(140,75,"Anyway","Roboto_BlackItalic24",badge.WHITE)
ugfx.flush()
```
More info on the [MicroPython badge features](https://wiki.sha2017.org/w/Projects:Badge/MicroPython)

