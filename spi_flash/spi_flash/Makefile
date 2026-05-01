CC=gcc
EXEC=spi_flash
#SOURCE=spi_flash.cpp get_opt.c memory.c percent_calc.c s25fl512s.c conv_str_to_uint32.c
OBJ=spi_flash.o get_opt.o memory.o percent_calc.o s25fl512s.o conv_str_to_uint32.o w25q32.o w25q64fv.o m25p16.o m25p64.o m25p128.o st_m25p80.o
#CFLAGS=-Wall -I. -L.
CFLAGS=-Wall -std=c99
#LDFLAGS=
LDLIB=-L. -ldl
# -lMPSSE

all: clean $(EXEC)
	@echo "=COMPILE========================================================================"
#	$(CC) $(CFLAGS) $(OBJ) -o $(NAME) $(LDFLAGS)

$(EXEC):$(OBJ)
	@echo "=COMPILE========================================================================"
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ) $(LDLIB)

clean:
	@echo "=CLEAN=========================================================================="
	rm -fr *.o 
	rm -fr $(EXEC)

