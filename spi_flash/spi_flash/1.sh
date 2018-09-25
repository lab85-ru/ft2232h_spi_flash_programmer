#!/bin/sh
sudo rmmod ftdi_sio
sudo rmmod usbserial
#sudo ./spi_flash --chip s25fl512s --read 1.bin --offset 0 --size 1000000
#sudo ./spi_flash --chip s25fl512s --write 1.tar.gz --offset 0
sudo ./spi_flash --chip s25fl512s --read 1.bin --offset 0 --size 28857503 --clk 25000000
sudo ./spi_flash --chip s25fl512s --cmp 1.tar.gz --offset 0 --clk 25000000
