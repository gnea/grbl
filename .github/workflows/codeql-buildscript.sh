#!/usr/bin/env bash

sudo apt-get -y install avrdude avr-libc gcc-avr make unzip
make grbl.hex
