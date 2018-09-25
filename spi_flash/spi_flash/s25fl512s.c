#include <stdio.h>
#include <stdint.h>
#include "memory.h"
#include "ftd2xx.h"
#include "libMPSSE_spi.h"

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


extern const struct mem_chip_st chip_s25fl512s;
// function ===================================================================
static FT_STATUS id_read( FT_HANDLE ftHandle );
static FT_STATUS read(FT_HANDLE ftHandle, const uint32 adr_start, uint8 *buf, const uint32 len, uint32 * sizeTransfered);
static FT_STATUS write(FT_HANDLE ftHandle, const uint32 adr_start, uint8 *buf, const uint32 prog_buf_size, uint32 * sizeTransfered);
static FT_STATUS erase( FT_HANDLE ftHandle, const uint32 adr_sec);
static FT_STATUS wren(FT_HANDLE ftHandle );
static FT_STATUS wrdi(FT_HANDLE ftHandle );
static int get_status(FT_HANDLE ftHandle );
//=============================================================================

//struct mem_chip_st
//{
//    char name[ NAME_SIZE ];   // chip name "S25FL512S"
//    uint32_t density;         // razmer flash chipa v bytes
//    uint32_t sector_erase_size; 
//    uint32_t prog_buf_size;
//    int (*id_read)( FT_HANDLE ftHandle );
//    int (*read)(const uint32_t adr, uint8_t *buf, const uint32_t size);
//    int (*write)(const uint32_t adr, uint8_t *buf, const uint32_t size);
//    int (*erase)(const uint32_t adr);
//};


const struct mem_chip_st chip_s25fl512s = {
    "s25fl512s",  // name
	64*1024*1024, // density(bytes)
	256*1024,     // sector_erase_size(bytes)
	512,          // prog_buf_size(bytes)
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
#define CMD_REMS       (0x90)

#define MANUFACTURE_ID (0x01)
#define DEVICE_ID      (0x19)

static FT_STATUS id_read( FT_HANDLE ftHandle )
{
	uint32 sizeToTransfer = 0;
	uint32 sizeTransfered;
	FT_STATUS status;
	uint8 buf[ 5 ];

	printf("S25FL512S ID READ.\r\n");

	p_SPI_Read = (pfunc_SPI_Read)GET_FUN_POINTER(h_libMPSSE, "SPI_Read");
	CHECK_NULL(p_SPI_Read);
	p_SPI_Write = (pfunc_SPI_Write)GET_FUN_POINTER(h_libMPSSE, "SPI_Write");
	CHECK_NULL(p_SPI_Write);


	buf[ 0 ] = CMD_REMS;
	buf[ 1 ] = 0;
	buf[ 2 ] = 0;
	buf[ 3 ] = 0;

	// send CMD REMS + 3 byte addr=0
	sizeToTransfer = 4;
	sizeTransfered = 0;
	status = p_SPI_Write(ftHandle, buf, sizeToTransfer, &sizeTransfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
	APP_CHECK_STATUS(status);

	/*Read 2 bytes*/
	sizeToTransfer=2;
	sizeTransfered=0;
	status = p_SPI_Read(ftHandle, buf, sizeToTransfer, &sizeTransfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
	APP_CHECK_STATUS(status);

	printf("Manufacture ID = 0x%x\n\r", buf[0]);
	printf("Device ID      = 0x%x\n\r", buf[1]);
    printf("Chip S25FL512S ");
	if (MANUFACTURE_ID == buf[0] && DEVICE_ID == buf[1]){
        printf(" found.\r\n");
		return FT_OK;
	} else {
		printf("is not found.\r\n");
        return FT_DEVICE_NOT_FOUND;
	}
}
//=============================================================================
// Read data from chip
//=============================================================================
#define CMD_4READ       (0x13)
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

	cmd[ 0 ] = CMD_4READ;
	cmd[ 1 ] = (adr_start >> 24) & 0xff;
	cmd[ 2 ] = (adr_start >> 16) & 0xff;
	cmd[ 3 ] = (adr_start >> 8)  & 0xff;
	cmd[ 4 ] = (adr_start >> 0)  & 0xff;

	// send CMD REMS + 4 byte addr
	sizeToTransfer = 5;
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
#define CMD_4PP       (0x12)
static FT_STATUS write(FT_HANDLE ftHandle, const uint32 adr_start, uint8 *buf, const uint32 size_for_write, uint32 * sizeTransfered)
{
	uint32 sizeToTransfer = 0;
	uint32 n = 0;
	FT_STATUS status;
	uint8 cmd[ 5 ];

	if ((adr_start & (chip_s25fl512s.prog_buf_size - 1)) != 0){
		printf("ERROR: (write) Set start adr != n*prog_buf_size.\n\r");
		return FT_INVALID_PARAMETER;
	}

	p_SPI_Read = (pfunc_SPI_Read)GET_FUN_POINTER(h_libMPSSE, "SPI_Read");
	CHECK_NULL(p_SPI_Read);
	p_SPI_Write = (pfunc_SPI_Write)GET_FUN_POINTER(h_libMPSSE, "SPI_Write");
	CHECK_NULL(p_SPI_Write);

	cmd[ 0 ] = CMD_4PP;
	cmd[ 1 ] = (adr_start >> 24) & 0xff;
	cmd[ 2 ] = (adr_start >> 16) & 0xff;
	cmd[ 3 ] = (adr_start >> 8)  & 0xff;
	cmd[ 4 ] = (adr_start >> 0)  & 0xff;

	// send CMD REMS + 4 byte addr
	sizeToTransfer = 5;
	*sizeTransfered = 0;
	n = 0;
	status = p_SPI_Write(ftHandle, cmd, sizeToTransfer, &n, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
	if (status != FT_OK){
		printf("ERORR: (write) p_SPI_Write return = %d\n\r", status);
	}
	//*sizeTransfered = n;

	// send data len = prog_buf_size
	sizeToTransfer = size_for_write;
    status = p_SPI_Write(ftHandle, buf, sizeToTransfer, &n, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
	if (status != FT_OK){
		printf("ERORR: (write) p_SPI_Write return = %d\n\r", status);
	}
	//*sizeTransfered = *sizeTransfered + n;
	*sizeTransfered = n;

	return status;
}

//=============================================================================
// Erase sector (chip)
//=============================================================================
#define CMD_4SE       (0xdc)
static FT_STATUS erase( FT_HANDLE ftHandle, const uint32 adr_sec)
{
	uint32 sizeToTransfer = 0;
	uint32 sizeTransfered;
	FT_STATUS status;
	uint8 cmd[ 5 ];

	p_SPI_Write = (pfunc_SPI_Write)GET_FUN_POINTER(h_libMPSSE, "SPI_Write");
	CHECK_NULL(p_SPI_Write);

	if (adr_sec % chip_s25fl512s.sector_erase_size){ // adr ne virovene na granicu sectora !!!
		printf("ERROR: set addres sector error != n * sec_erase_size.\n\r");
		return FT_INVALID_PARAMETER;
	}

	cmd[ 0 ] = CMD_4SE;
	cmd[ 1 ] = (adr_sec >> 24) & 0xff;
	cmd[ 2 ] = (adr_sec >> 16) & 0xff;
	cmd[ 3 ] = (adr_sec >> 8)  & 0xff;
	cmd[ 4 ] = (adr_sec >> 0)  & 0xff;

	// send CMD + 4 byte addr
	sizeToTransfer = 5;
	sizeTransfered = 0;
	status = p_SPI_Write(ftHandle, cmd, sizeToTransfer, &sizeTransfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
	APP_CHECK_STATUS(status);

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

	// send CMD WREN
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
#define CMD_RDSR1       (0x05)
#define FLAG_WIP        (1<<0)
#define FLAG_P_ERR      (1<<6)
#define FLAG_E_ERR      (1<<5)
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

	cmd[ 0 ] = CMD_RDSR1;

	// send CMD RDSR1
	sizeToTransfer = 1;
	sizeTransfered = 0;
	status = p_SPI_Write(ftHandle, cmd, sizeToTransfer, &sizeTransfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
	APP_CHECK_STATUS(status);

    // get status
	sizeToTransfer = 1;
    sizeTransfered=0;
    status = p_SPI_Read(ftHandle, cmd, sizeToTransfer, &sizeTransfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    APP_CHECK_STATUS(status);

	if (cmd[0] & FLAG_P_ERR || cmd[0] & FLAG_E_ERR){
		printf("ERROR: status FLAG_P_ERR || FLAG_E_ERR\n\r");
		return -1;
	}

	if (cmd[0] & FLAG_WIP) return 1;
	else return 0;
}
