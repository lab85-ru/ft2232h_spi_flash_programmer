#include <stdio.h>
#include <stdint.h>
#include "memory.h"
#include "ftd2xx.h"
#include "libMPSSE_spi.h"

#ifdef _WIN32

#else
#include <unistd.h>
#endif


typedef FT_STATUS (*pfunc_SPI_Read)(FT_HANDLE handle, uint8 *buffer, \
	uint32 sizeToTransfer, uint32 *sizeTransfered, uint32 options);
extern pfunc_SPI_Read p_SPI_Read;

typedef FT_STATUS (*pfunc_SPI_Write)(FT_HANDLE handle, uint8 *buffer, \
	uint32 sizeToTransfer, uint32 *sizeTransfered, uint32 options);
extern pfunc_SPI_Write p_SPI_Write;


#ifdef _WIN32
#ifdef _MSC_VER
extern HMODULE h_libMPSSE;
#else
extern HANDLE h_libMPSSE;
#endif
#endif
#ifdef __linux
extern void *h_libMPSSE;
#include <dlfcn.h>
#endif

#ifdef _WIN32
	#define GET_FUN_POINTER	GetProcAddress
	#define CHECK_ERROR(exp) {if(exp==NULL){printf("%s:%d:%s():  NULL \
expression encountered \n",__FILE__, __LINE__, __FUNCTION__);exit(1);}else{;}};
#endif

#ifdef __linux
	#define GET_FUN_POINTER	dlsym
	#define CHECK_ERROR(exp) {if(dlerror() != NULL){printf("line %d: ERROR \
dlsym\n",__LINE__);}}
#endif


extern const struct mem_chip_st chip_w25q32;
// function ===================================================================
static FT_STATUS id_read( FT_HANDLE ftHandle );
static FT_STATUS read(FT_HANDLE ftHandle, const uint32 adr_start, uint8 *buf, const uint32 len, uint32 * sizeTransfered);
static FT_STATUS write(FT_HANDLE ftHandle, const uint32 adr_start, uint8 *buf, const uint32 prog_buf_size, uint32 * sizeTransfered);
static FT_STATUS erase( FT_HANDLE ftHandle, const uint32 adr_sec);
static FT_STATUS wren(FT_HANDLE ftHandle );
static FT_STATUS wrdi(FT_HANDLE ftHandle );
static int get_status(FT_HANDLE ftHandle );
//=================================== ==========================================

const struct mem_chip_st chip_w25q32 = {
    "W25Q32",     // name
	4*1024*1024,  // density(bytes)
	4*1024,       // sector_erase_size(bytes)
	256,          // prog_buf_size(bytes)
	id_read,      //(*id_read)
	read,         //(*read)
	write,        //(*write)
	erase,        //(*erase)
	wren,         //(*wr_en)
	wrdi,         //(*wr_dis)
	get_status    //(*get_status)

};

//=============================================================================
// Read ID from chip
//=============================================================================
#define CMD_JEDEC_ID    (0x9f) // Read EF 40 16
#define CMD_MANUFACTURED_DEVICE_ID (0x90)

#define W25Q32BV_ID     (0x15) // W25Q32BV
#define MANUFACTURE_ID  (0xEF) // Winbond Serial Flash
#define MEM_TYPE_ID     (0x40)
#define MEM_CAPACITY_ID (0x16)

static FT_STATUS id_read( FT_HANDLE ftHandle )
{
	uint32 sizeToTransfer = 0;
	uint32 sizeTransfered;
	FT_STATUS status;
	uint8 buf[ 5 ];

	printf("W25Q32 ID READ.\r\n");

	p_SPI_Read = (pfunc_SPI_Read)GET_FUN_POINTER(h_libMPSSE, "SPI_Read");
	CHECK_NULL(p_SPI_Read);
	p_SPI_Write = (pfunc_SPI_Write)GET_FUN_POINTER(h_libMPSSE, "SPI_Write");
	CHECK_NULL(p_SPI_Write);


	buf[ 0 ] = CMD_JEDEC_ID;
	buf[ 1 ] = 0;
	buf[ 2 ] = 0;
	buf[ 3 ] = 0;

	// send CMD JEDEC ID
	sizeToTransfer = 1;
	sizeTransfered = 0;
	status = p_SPI_Write(ftHandle, buf, sizeToTransfer, &sizeTransfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
	APP_CHECK_STATUS(status);

	/*Read 3 bytes*/
	sizeToTransfer = 3;
	sizeTransfered = 0;
	status = p_SPI_Read(ftHandle, buf, sizeToTransfer, &sizeTransfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
	APP_CHECK_STATUS(status);

	printf("Manufacture ID  = 0x%02X\n\r", buf[0]);
	printf("Mem type ID     = 0x%02X\n\r", buf[1]);
    printf("Mem capacity ID = 0x%02X\n\r", buf[2]);
    printf("Chip W25Q32 ");
	if (MANUFACTURE_ID == buf[0] && MEM_TYPE_ID == buf[1] && MEM_CAPACITY_ID == buf[2]){
        printf("found.\r\n");
		return FT_OK;
	} else {
		printf("is not found.\r\n");
        return FT_DEVICE_NOT_FOUND;
	}
}
//=============================================================================
// Read data from chip
//=============================================================================
#define CMD_READ       (0x03)
#define READ_LEN        (128) // kolichestvo byte na chetie dla 1 operacii SPI_read
//#define READ_LEN        (512) // kolichestvo byte na chetie dla 1 operacii SPI_read
//#define READ_LEN        (8192) // kolichestvo byte na chetie dla 1 operacii SPI_read
static FT_STATUS read(FT_HANDLE ftHandle, const uint32 adr_start, uint8 *buf, const uint32 len, uint32 * sizeTransfered)
{
	uint32 sizeToTransfer = 0;
	uint32 size_Transfered;
	FT_STATUS status;
	uint8 cmd[ 5 ];

	p_SPI_Read = (pfunc_SPI_Read)GET_FUN_POINTER(h_libMPSSE, "SPI_Read");
	CHECK_NULL(p_SPI_Read);
	p_SPI_Write = (pfunc_SPI_Write)GET_FUN_POINTER(h_libMPSSE, "SPI_Write");
	CHECK_NULL(p_SPI_Write);

	cmd[ 0 ] = CMD_READ;
	cmd[ 1 ] = (adr_start >> 16) & 0xff;
	cmd[ 2 ] = (adr_start >> 8)  & 0xff;
	cmd[ 3 ] = (adr_start >> 0)  & 0xff;

	// send CMD READ + 3 byte addr
	sizeToTransfer = 4;
	*sizeTransfered = 0;
	status = p_SPI_Write(ftHandle, cmd, sizeToTransfer, &size_Transfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
	APP_CHECK_STATUS(status);

	sizeToTransfer = len;
    status = p_SPI_Read(ftHandle, buf, sizeToTransfer, sizeTransfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    APP_CHECK_STATUS(status);

	return status;
}

//=============================================================================
// Write data to chip
// adr_start kraten -> prog_buf_size
//=============================================================================
#define CMD_PP       (0x02)
static FT_STATUS write(FT_HANDLE ftHandle, const uint32 adr_start, uint8 *buf, const uint32 size_for_write, uint32 * sizeTransfered)
{
	uint32 sizeToTransfer = 0;
	uint32 n = 0;
	FT_STATUS status;
	uint8 cmd[ 5 ];

	if ((adr_start & (chip_w25q32.prog_buf_size - 1)) != 0){
		printf("ERROR: (write) Set start adr != n*prog_buf_size.\n\r");
		return FT_INVALID_PARAMETER;
	}

	p_SPI_Read = (pfunc_SPI_Read)GET_FUN_POINTER(h_libMPSSE, "SPI_Read");
	CHECK_NULL(p_SPI_Read);
	p_SPI_Write = (pfunc_SPI_Write)GET_FUN_POINTER(h_libMPSSE, "SPI_Write");
	CHECK_NULL(p_SPI_Write);

	cmd[ 0 ] = CMD_PP;
	cmd[ 1 ] = (adr_start >> 16) & 0xff;
	cmd[ 2 ] = (adr_start >> 8)  & 0xff;
	cmd[ 3 ] = (adr_start >> 0)  & 0xff;

	// send CMD PP + 3 byte addr
	sizeToTransfer = 4;
	*sizeTransfered = 0;
	n = 0;
	status = p_SPI_Write(ftHandle, cmd, sizeToTransfer, &n, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
	if (status != FT_OK){
		printf("ERORR: (write) p_SPI_Write return = %d\n\r", status);
	}

	// send data len = prog_buf_size
	sizeToTransfer = size_for_write;
    status = p_SPI_Write(ftHandle, buf, sizeToTransfer, &n, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
	if (status != FT_OK){
		printf("ERORR: (write) p_SPI_Write return = %d\n\r", status);
	}
	*sizeTransfered = n;

	return status;
}

//=============================================================================
// Erase sector (chip)
//=============================================================================
#define CMD_SE       (0x20)//(0xd8)
static FT_STATUS erase( FT_HANDLE ftHandle, const uint32 adr_sec)
{
	uint32 sizeToTransfer = 0;
	uint32 sizeTransfered;
	FT_STATUS status;
	uint8 cmd[ 5 ];

	p_SPI_Write = (pfunc_SPI_Write)GET_FUN_POINTER(h_libMPSSE, "SPI_Write");
	CHECK_NULL(p_SPI_Write);

	if (adr_sec % chip_w25q32.sector_erase_size){ // adr ne virovene na granicu sectora !!!
		printf("ERROR: set addres sector error != n * sec_erase_size.\n\r");
		return FT_INVALID_PARAMETER;
	}

	cmd[ 0 ] = CMD_SE;
	cmd[ 1 ] = (adr_sec >> 16) & 0xff;
	cmd[ 2 ] = (adr_sec >> 8)  & 0xff;
	cmd[ 3 ] = (adr_sec >> 0)  & 0xff;

	// send CMD SE + 3 byte addr
	sizeToTransfer = 4;
	sizeTransfered = 0;
	status = p_SPI_Write(ftHandle, cmd, sizeToTransfer, &sizeTransfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
	APP_CHECK_STATUS(status);

/*
#ifdef _WIN32
	Sleep(45); // Sector Erase Time (4KB) tSE nom=45 max=400 ms
#else
	usleep(45000);
#endif
*/

	return status;
}

//=============================================================================
// Write_enable (chip)
//=============================================================================
#define CMD_WREN       (0x06)
static FT_STATUS wren( FT_HANDLE ftHandle )
{
	uint32 sizeToTransfer = 0;
	uint32 sizeTransfered;
	FT_STATUS status;
	uint8 cmd[ 5 ];

	p_SPI_Write = (pfunc_SPI_Write)GET_FUN_POINTER(h_libMPSSE, "SPI_Write");
	CHECK_NULL(p_SPI_Write);

	cmd[ 0 ] = CMD_WREN;

	// send CMD WREN
	sizeToTransfer = 1;
	sizeTransfered = 0;
	status = p_SPI_Write(ftHandle, cmd, sizeToTransfer, &sizeTransfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
	APP_CHECK_STATUS(status);

	return status;
}

//=============================================================================
// Write_disable (chip)
//=============================================================================
#define CMD_WRDI       (0x04)
static FT_STATUS wrdi( FT_HANDLE ftHandle )
{
	uint32 sizeToTransfer = 0;
	uint32 sizeTransfered;
	FT_STATUS status;
	uint8 cmd[ 5 ];

	p_SPI_Write = (pfunc_SPI_Write)GET_FUN_POINTER(h_libMPSSE, "SPI_Write");
	CHECK_NULL(p_SPI_Write);

	cmd[ 0 ] = CMD_WRDI;

	// send CMD WRDI
	sizeToTransfer = 1;
	sizeTransfered = 0;
	status = p_SPI_Write(ftHandle, cmd, sizeToTransfer, &sizeTransfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
	APP_CHECK_STATUS(status);

	return status;
}

//=============================================================================
// get_status (busy)
// return
// -1 error prog - erase
// 0 - ready
// 1 - busy
//=============================================================================
#define CMD_RDSR       (0x05)
#define FLAG_WIP       (1<<0)
static int get_status(FT_HANDLE ftHandle )
{
	uint32 sizeToTransfer = 0;
	uint32 sizeTransfered;
	FT_STATUS status;
	uint8 cmd[ 2 ];

	p_SPI_Write = (pfunc_SPI_Write)GET_FUN_POINTER(h_libMPSSE, "SPI_Write");
	CHECK_NULL(p_SPI_Write);

	p_SPI_Read = (pfunc_SPI_Read)GET_FUN_POINTER(h_libMPSSE, "SPI_Read");
	CHECK_NULL(p_SPI_Read);

	cmd[ 0 ] = CMD_RDSR;

	// send CMD RDSR
	sizeToTransfer = 1;
	sizeTransfered = 0;
	status = p_SPI_Write(ftHandle, cmd, sizeToTransfer, &sizeTransfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
	APP_CHECK_STATUS(status);

    // get status
	sizeToTransfer = 1;
    sizeTransfered=0;
    status = p_SPI_Read(ftHandle, cmd, sizeToTransfer, &sizeTransfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    APP_CHECK_STATUS(status);

	if (cmd[0] & FLAG_WIP) return 1;
	else return 0;
}
