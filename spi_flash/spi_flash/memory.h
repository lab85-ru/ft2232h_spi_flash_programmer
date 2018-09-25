#ifndef MEMORY_H_
#define MEMORY_H_



#include <stdlib.h>
#include <stdint.h>
//#include "ftd2xx.h"
#include "libMPSSE_spi.h"

#ifdef __MSC_VER
#include <windows.h>
#endif

#define NAME_SIZE (64)

struct mem_chip_st
{
    char name[ NAME_SIZE ];   // chip name "S25FL512S"
    uint32 density;         // razmer flash chipa v bytes
    uint32 sector_erase_size; 
    uint32 prog_buf_size;
    FT_STATUS (*id_read)( FT_HANDLE ftHandle );
    FT_STATUS (*read)(FT_HANDLE ftHandle, const uint32 adr_start, uint8 *buf, const uint32 len, uint32 * sizeTransfered);
    FT_STATUS (*write)(FT_HANDLE ftHandle, const uint32 adr_start, uint8 *buf, const uint32 prog_buf_size, uint32 * sizeTransfered);
    FT_STATUS (*erase)(FT_HANDLE ftHandle, const uint32 adr_sec);
	FT_STATUS (*wren)(FT_HANDLE ftHandle);
	FT_STATUS (*wrdi)(FT_HANDLE ftHandle);
	int (*get_status)(FT_HANDLE ftHandle);


};

extern const struct mem_chip_st *chip_array[];
extern const unsigned int CHIP_QUANTITY;




#define APP_CHECK_STATUS(exp) {if(exp!=FT_OK){printf("%s:%d:%s(): status(0x%x) \
!= FT_OK\n",__FILE__, __LINE__, __FUNCTION__,exp);exit(1);}else{;}};

#define CHECK_NULL(exp){if(exp==NULL){printf("%s:%d:%s():  NULL expression \
encountered \n",__FILE__, __LINE__, __FUNCTION__);exit(1);}else{;}};





#endif