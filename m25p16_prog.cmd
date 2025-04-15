@ECHO OFF
hex2bin -c top.mcs
spi_flash --chip M25P16 --write top.bin --clk 1000000 --channel 0
spi_flash --chip M25P16 --cmp top.bin --clk 1000000 --offset 0 --channel 0

