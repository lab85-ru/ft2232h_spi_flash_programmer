@ECHO OFF
hex2bin -c top.mcs

spi_flash --chip M25P16 --write top.bin --clk 1000000 --channel 0
if %ErrorLevel% equ 0 goto compare
goto error_write

:compare
spi_flash --chip M25P16 --cmp top.bin --clk 1000000 --offset 0 --channel 0
if %ErrorLevel% equ 0 goto exit
goto error_compare

:exit
echo "================================================================================"
echo "---- OK ----"
echo "================================================================================"
exit 0

:error_write
echo "================================================================================"
echo "---- ERROR WRITE ----"
echo "================================================================================"
exit 1

:error_compare
echo "================================================================================"
echo "---- ERROR COMPARE ----"
echo "================================================================================"
exit 1
