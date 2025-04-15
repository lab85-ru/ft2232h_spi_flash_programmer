// spi_flash.cpp : Defines the entry point for the console application.
//
/******************************************************************************/
/* 							 Include files										   */
/******************************************************************************/
/* Standard C libraries */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* OS specific libraries */
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux
#include <dlfcn.h>
#endif

/* Include D2XX header*/
#include "ftd2xx.h"

/* Include libMPSSE header */
#include "libMPSSE_spi.h"

#include "memory.h"
#include "conv_str_to_uint32.h"
#include "percent_calc.h"
#include "get_opt.h"
#include "git_commit.h"

int chip_read(FT_HANDLE ftHandle, const struct mem_chip_st *ch, const uint32_t adr_start, uint8_t *buf, const uint32_t len);
int chip_erase(FT_HANDLE ftHandle, const struct mem_chip_st *ch, const uint32 adr_start, const uint32 len);
int chip_write(FT_HANDLE ftHandle, const struct mem_chip_st *ch, const uint32 adr_start, uint8 *buf, const uint32 len);

const char PRINT_HELP[] = {
"----------------------------------------------------------------------------\n\r"
"- Prog for chip FTx232x. Read/Write file to SPI flash.----------------------\n\r"
"    Parameters:\n\r"
" --chiplist           - print all chips supported.\n\r"
" --channel <channel>  - FT2232H channel, 0 or 1.\n\r"
" --chip   <chipname>  - set \"name\"chip.\n\r"
" --offset <offset>    - set start address(format hex = 0x00000000 or dec).\n\r"
" --size   <len>       - set length(format hex = 0x00000000 or dec).\n\r"
" --read   <file-name> - operation READ flash -> write to file.\n\r"
" --write  <file-name> - operation file to -> WRITE flash.\n\r"
" --erase              - erase flash(set --size & --offset).\n\r"
" --clk                - set clk for spi (Hz)\n\r"
" --cmp <file-name>    - compare FLASH & FILE, set --offset.\n\r"
"----------------------------------------------------------------------------\n\r"
};

const char PRINT_TIRE[] = {"================================================================================\n\r"};
const char PRINT_PROG_NAME[] = {" FTx232x SPI FLASH MEMORY PROGRAMMATOR\n\r"};
const char PRINT_VERSION[] = {" Ver 1.3 2025. Sviridov Georgy. sgot@inbox.ru\n\r"};


param_opt_st param_opt;



/******************************************************************************/
/*								Macro and type defines							   */
/******************************************************************************/
/* Helper macros */
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

typedef FT_STATUS (*pfunc_SPI_GetNumChannels)(uint32 *numChannels);
pfunc_SPI_GetNumChannels p_SPI_GetNumChannels;

typedef FT_STATUS (*pfunc_SPI_GetChannelInfo)(uint32 index, \
	FT_DEVICE_LIST_INFO_NODE *chanInfo);
pfunc_SPI_GetChannelInfo p_SPI_GetChannelInfo;

typedef FT_STATUS (*pfunc_SPI_OpenChannel)(uint32 index, FT_HANDLE *handle);
pfunc_SPI_OpenChannel p_SPI_OpenChannel;

typedef FT_STATUS (*pfunc_SPI_InitChannel)(FT_HANDLE handle, \
	ChannelConfig *config);
pfunc_SPI_InitChannel p_SPI_InitChannel;

typedef FT_STATUS (*pfunc_SPI_CloseChannel)(FT_HANDLE handle);
pfunc_SPI_CloseChannel p_SPI_CloseChannel;

typedef FT_STATUS (*pfunc_SPI_Read)(FT_HANDLE handle, uint8 *buffer, \
	uint32 sizeToTransfer, uint32 *sizeTransfered, uint32 options);
pfunc_SPI_Read p_SPI_Read;

typedef FT_STATUS (*pfunc_SPI_Write)(FT_HANDLE handle, uint8 *buffer, \
	uint32 sizeToTransfer, uint32 *sizeTransfered, uint32 options);
pfunc_SPI_Write p_SPI_Write;

typedef FT_STATUS (*pfunc_SPI_IsBusy)(FT_HANDLE handle, bool *state);
pfunc_SPI_IsBusy p_SPI_IsBusy;

typedef FT_STATUS (*pfunc_SPI_ReadWrite)(FT_HANDLE handle, uint8 *inBuffer,
	uint8 *outBuffer, uint32 sizeToTransfer, uint32 *sizeTransferred,
	uint32 transferOptions);
pfunc_SPI_ReadWrite p_SPI_ReadWrite;



/* Application specific macro definations */
//#define SPI_WRITE_COMPLETION_RETRY		10
//#define START_ADDRESS_EEPROM 	0x00 /*read/write start address inside the EEPROM*/
//#define END_ADDRESS_EEPROM		0x10
//#define RETRY_COUNT_EEPROM		10	/* number of retries if read/write fails */
//#define CHANNEL_TO_OPEN			0	/*0 for first available channel, 1 for next... */
//#define SPI_SLAVE_0				0
//#define SPI_SLAVE_1				1
//#define SPI_SLAVE_2				2
//#define DATA_OFFSET				4
//#define USE_WRITEREAD			0

/******************************************************************************/
/*								Global variables							  	    */
/******************************************************************************/
static FT_HANDLE ftHandle;

#ifdef _WIN32
#ifdef _MSC_VER
	HMODULE h_libMPSSE;
#else
	HANDLE h_libMPSSE;
#endif
#endif
#ifdef __linux
	void *h_libMPSSE;
#endif


#define FILE_RW_BLOCK_SIZE (1024)


//=============================================================================
// MAIN
//=============================================================================
int main(int argc, char* argv[])
{
	struct stat stat_buf;
	FILE * fi = 0;
	int res = 0;
	FT_STATUS status = FT_OK;
	FT_DEVICE_LIST_INFO_NODE devList = {0};
	ChannelConfig channelConf = {0};
	uint32 channels = 0;
	uint8 latency = 255;	
	uint8 *mem = NULL;   // ukazatel na buf-mem dla READ_WRITE SPI mem.
	uint8 *mem_cmp = NULL;   // ukazatel na buf-mem dla COMPARE FLASH - FILE.

	param_opt_st param_opt;

	size_t pos = 0;
    size_t r_size = 0;
    size_t w_size = 0;
    size_t result = 0;

	param_opt.chip_list   = 0;
    param_opt.chip_name   = 0;
    param_opt.chip_n      = 0;
    param_opt.file_name   = 0;
    param_opt.offset      = 0;
	param_opt.size        = 0;
    param_opt.op          = OP_NONE;
	param_opt.clk         = 0;
	param_opt.channel     = 0;

	printf( PRINT_TIRE );
    printf( PRINT_PROG_NAME );
	printf( PRINT_VERSION );
    printf( " GIT = %s\n\r", git_commit_str );
	printf( PRINT_TIRE );

    // razbor perametrov
	if (argc == 1){
	    printf( PRINT_HELP );
		return 0;
	}

	//for (int i=0;i<argc;i++){
	//	printf("argv[ %d ] = %s\n\r", i, argv[i]);
	//}

	res = get_opt(argc, argv, &param_opt);
	if (res == -1){
		printf("\n\rERROR: input parameters.\n\r");
		return 1;
	}

	// print spisol mikroshem
	if (param_opt.chip_list != 0){
		printf("\n\r--- CHIP LIST ---\n\r");
	    for(int i=0; i!=CHIP_QUANTITY; i++){
			printf("CHIP: %s\n\r", chip_array[ i ]->name);
	    }
        printf("\n\r");
		return 0;
	}


    // proverka nalichiya mikroshemi v spiske !

	if (param_opt.chip_name != 0){
		int i = 0;
		while(strcmp(param_opt.chip_name, chip_array[ i ]->name) != 0){
            i++;
			if (i == CHIP_QUANTITY){
		        printf("ERROR: Chip name %s not found.\n\r", param_opt.chip_name);
		        return 1;
			}
		}
		param_opt.chip_n = i;
	} else {
		printf("ERROR: Chip name %s not found.\n\r", param_opt.chip_name);
		return 1;
	}

	if (param_opt.clk == 0){
	    printf("SPI CLK not defined, SET CLK = 1 MHz.\r\n");
		param_opt.clk = 1000000; // set 1 MHz
	}

	switch(param_opt.op){

	case OP_NONE:{
		printf("Operation action is not set !!!\n\r");
		return 1;
	}

	case OP_RD:{
		if (param_opt.size == 0){
			printf("ERROR: size = 0, the value should be zero.\n\r");
			return 1;
		}
		if (param_opt.file_name == 0){
		    printf("ERROR: File name is not set.\n\r");
		    return 1;
	    }
        break;
	}

	case OP_WR:{
		if (param_opt.file_name == 0){
		    printf("ERROR: File name is not set.\n\r");
		    return 1;
	    }
        break;
	}

	case OP_CMP:{
		if (param_opt.file_name == 0){
		    printf("ERROR: File name is not set.\n\r");
		    return 1;
	    }
        break;
	}

	case OP_ER:{
		if (param_opt.size == 0){
			printf("ERROR: size = 0, the value should be zero.\n\r");
			return 1;
		}
        break;
	}

	default:{
		printf("Operation action is not set !!!\n\r");
		return 1;
	}

	}//switch(param_opt.op){




	/* load library */
#ifdef _WIN32
	#ifdef _MSC_VER
		//h_libMPSSE = LoadLibrary(L"libMPSSE.dll");
		h_libMPSSE = LoadLibrary("libMPSSE.dll");
	#else
		h_libMPSSE = LoadLibrary("libMPSSE.dll");
	#endif
	if(NULL == h_libMPSSE)
	{
		printf("Failed loading libMPSSE.dll. \
		\nPlease check if the file exists in the working directory\n");
		exit(1);
	}
#endif

#if __linux
	h_libMPSSE = dlopen("libMPSSE.so",RTLD_LAZY);
	if(!h_libMPSSE)
	{
		printf("Failed loading libMPSSE.so. Please check if the file exists in \
the shared library folder(/usr/lib or /usr/lib64)\n");
		exit(1);
	}
#endif
	/* init function pointers */
	p_SPI_GetNumChannels = (pfunc_SPI_GetNumChannels)GET_FUN_POINTER(h_libMPSSE, "SPI_GetNumChannels");
	CHECK_NULL (p_SPI_GetNumChannels);
	p_SPI_GetChannelInfo = (pfunc_SPI_GetChannelInfo)GET_FUN_POINTER(h_libMPSSE, "SPI_GetChannelInfo");
	CHECK_NULL(p_SPI_GetChannelInfo);
	p_SPI_OpenChannel = (pfunc_SPI_OpenChannel)GET_FUN_POINTER(h_libMPSSE, "SPI_OpenChannel");
	CHECK_NULL(p_SPI_OpenChannel);
	p_SPI_InitChannel = (pfunc_SPI_InitChannel)GET_FUN_POINTER(h_libMPSSE, "SPI_InitChannel");
	CHECK_NULL(p_SPI_InitChannel);
	p_SPI_Read = (pfunc_SPI_Read)GET_FUN_POINTER(h_libMPSSE, "SPI_Read");
	CHECK_NULL(p_SPI_Read);
	p_SPI_Write = (pfunc_SPI_Write)GET_FUN_POINTER(h_libMPSSE, "SPI_Write");
	CHECK_NULL(p_SPI_Write);
	p_SPI_CloseChannel = (pfunc_SPI_CloseChannel)GET_FUN_POINTER(h_libMPSSE, "SPI_CloseChannel");
	CHECK_NULL(p_SPI_CloseChannel);
	p_SPI_IsBusy = (pfunc_SPI_IsBusy)GET_FUN_POINTER(h_libMPSSE, "SPI_IsBusy");
	CHECK_NULL(p_SPI_IsBusy);
	p_SPI_ReadWrite = (pfunc_SPI_ReadWrite)GET_FUN_POINTER(h_libMPSSE, "SPI_ReadWrite");
	CHECK_NULL(p_SPI_ReadWrite);

	channelConf.ClockRate = param_opt.clk; // Hz
	channelConf.LatencyTimer = latency;
	channelConf.configOptions = SPI_CONFIG_OPTION_MODE0 | SPI_CONFIG_OPTION_CS_DBUS3 | SPI_CONFIG_OPTION_CS_ACTIVELOW;
	channelConf.Pin = 0x00000000;/*FinalVal-FinalDir-InitVal-InitDir (for dir 0=in, 1=out)*/

	status = p_SPI_GetNumChannels(&channels);
	APP_CHECK_STATUS(status);
	printf("Number of available SPI channels = %d\n",(int)channels);

	if(channels == 0) return 1;

	if (stat(param_opt.file_name, &stat_buf) == -1){
		printf("ERROR: fstat return error !\n\r");
		return 1;
	}

    printf(PRINT_TIRE);
	printf(" CHIP   : %s\n\r", chip_array[ param_opt.chip_n ]->name);
	printf(" CHANNEL: %d\n\r", param_opt.channel);
	printf(" CLK    : %d Hz\n\r", param_opt.clk);
    printf(" IO     : ");
	if (param_opt.op == OP_NONE) printf("NONE.\n\r");
	if (param_opt.op == OP_RD) printf("READ.\n\r");
	if (param_opt.op == OP_WR) printf("WRITE.\n\r");
	if (param_opt.op == OP_ER) printf("ERASE.\n\r");
	if (param_opt.op == OP_CMP) printf("COMPARE.\n\r");
	printf(" OFFSET : %d ( 0x%x )\n\r", param_opt.offset, param_opt.offset);
    printf(" SIZE   : %d ( 0x%x )\n\r", param_opt.size, param_opt.size);
	printf(" FILE   : %s  %lld bytes\n\r", param_opt.file_name, (long long)stat_buf.st_size);
    printf(PRINT_TIRE);

	status = p_SPI_GetChannelInfo(param_opt.channel, &devList);
	APP_CHECK_STATUS(status);

	printf(PRINT_TIRE);
    printf(" FTx232x\n\r");
	printf(PRINT_TIRE);
	printf("Information on channel number %d:\n", param_opt.channel);
	/* print the dev info */
	//printf(" Flags        = 0x%x\n",devList.Flags);
	//printf(" Type         = 0x%x\n",devList.Type);
	//printf(" ID           = 0x%x\n",devList.ID);
	//printf(" LocId        = 0x%x\n",devList.LocId);
	printf(" SerialNumber = %s\n",devList.SerialNumber);
	printf(" Description  = %s\n",devList.Description);
	//printf(" ftHandle     = 0x%x\n",(unsigned int)devList.ftHandle);/*is 0 unless open*/
	printf(PRINT_TIRE);

	/* Open the first available channel */
	status = p_SPI_OpenChannel(param_opt.channel, &ftHandle);
	APP_CHECK_STATUS(status);
	//printf("\nhandle=0x%x status=0x%x\n",(unsigned int)ftHandle,status);
	status = p_SPI_InitChannel(ftHandle,&channelConf);
	APP_CHECK_STATUS(status);

	printf(PRINT_TIRE);
	printf(" READ ID.\n\r");
	printf(PRINT_TIRE);
	status = chip_array[ param_opt.chip_n ]->id_read( ftHandle );
	if (status != FT_OK){
		printf("ERROR: read chip ID.\n\r"); 
	    return 1;
	}

	mem = (uint8*)malloc( chip_array[ param_opt.chip_n ]->density );
	if (mem == NULL){
		printf("ERROR: malloc( size[  %d  ])\n\r", chip_array[ param_opt.chip_n ]->density);
		return 1;
	}
        
    //=====================================================================
	// WRITE
    //=====================================================================
	if (param_opt.op == OP_WR){
		printf(PRINT_TIRE);
		// proverka nalichiya funct WRITE
		if (chip_array[ param_opt.chip_n ]->write == 0){
			printf("ERROR: function .WRITE not defined = 0\n\r");
			return 1;
		}

		if (stat(param_opt.file_name, &stat_buf) == -1){
			printf("ERROR: fstat return error !\n\r");
			return 1;
		}

		// proverka diapazona offset+size <= density
		if (param_opt.offset + stat_buf.st_size > chip_array[ param_opt.chip_n ]->density ){
			printf("ERROR: file is BIG !!! file_size( %lld ) > chip_size(+offset %lld )\n\r", (long long)stat_buf.st_size, (long long)(chip_array[ param_opt.chip_n ]->density - param_opt.offset));
			return 1;
		}

		printf("File size = %lld bytes.\n\r", (long long)stat_buf.st_size);

		if (stat_buf.st_size == 0){
			printf("ERROR: file is empty.\n\r");
			return 1;
		}

		printf("Read data from file.\n\r");
           // read data from file
		fi = fopen(param_opt.file_name, "rb");
		if ( fi == NULL ){
			printf("ERROR: open file.\n\r");
			return 1;
		}

		pos = 0;
		while(pos != stat_buf.st_size){
            if (stat_buf.st_size - pos > FILE_RW_BLOCK_SIZE)
				r_size = FILE_RW_BLOCK_SIZE;
			else
                r_size = stat_buf.st_size - pos;

            result = fread( &mem[pos], 1, r_size, fi);
			if (result != r_size){
				#ifdef _WIN32
			    printf("ERROR: read data from file. result( %d ) != fread( r_size( %u ))\n\r", result, r_size);
                #else
				printf("ERROR: read data from file. result( %zd ) != fread( r_size( %zu ))\n\r", result, r_size);
                #endif
				return 1;
			}

            pos = pos + r_size;
		}
        fclose( fi );

		#ifdef _WIN32
		printf("File load OK, len = %u \n\r", pos);
        #else
		printf("File load OK, len = %zu \n\r", pos);
        #endif

		printf(PRINT_TIRE);
		printf(" ERASE:\n\r");
        printf(PRINT_TIRE);

		status = chip_erase(ftHandle, chip_array[ param_opt.chip_n ], param_opt.offset, pos);
		if (status != FT_OK){
			printf("ERROR: Chip erase !\n\r");
			return 1;
		}

		printf(PRINT_TIRE);
		printf(" WRITE:\n\r");
		printf(PRINT_TIRE);

		status = chip_write(ftHandle, chip_array[ param_opt.chip_n ], param_opt.offset, mem, pos);
		status = FT_OK;
		if (status != FT_OK){
			printf("ERROR: IO.\n\r");
			return 1;
		}
	}

    //=====================================================================
    // READ
    //=====================================================================
	else if (param_opt.op == OP_RD){
		printf(PRINT_TIRE);
		printf(" READ:\n\r");
		printf(PRINT_TIRE);

		// proverka nalichiya funct READ
		if (chip_array[ param_opt.chip_n ]->read == 0){
			printf("ERROR: function .READ not defined\n\r");
			return 1;
		}
			
		// proverka diapazona offset+size <= density
		if (param_opt.offset + param_opt.size > chip_array[ param_opt.chip_n ]->density){
			printf("ERROR: start adr + size( %d ) > size chip( %d )\n\r", param_opt.offset + param_opt.size, chip_array[ param_opt.chip_n ]->density);
			return 1;
		}

        status = chip_read(ftHandle, chip_array[ param_opt.chip_n ], param_opt.offset, mem, param_opt.size);
		if (status != FT_OK){
			printf("ERROR: IO.\n\r");
			return 1;
		}

		printf(PRINT_TIRE);

		printf("Write data to file.\n\r");
        // write data to file
		fi = fopen(param_opt.file_name, "wb");
		if ( fi == NULL ){
			printf("ERROR: open file.\n\r");
			return 1;
		}

		while(pos != param_opt.size){
            if (param_opt.size - pos > FILE_RW_BLOCK_SIZE)
			    w_size = FILE_RW_BLOCK_SIZE;
			else
                w_size = param_opt.size - pos;

			result = fwrite( &mem[pos], 1, w_size, fi);
			if (result != w_size){
				#ifdef _WIN32
			    printf("ERROR: write data to file. result( %u ) != fwrite( w_size( %u ))\n\r", result, w_size);
                #else
                printf("ERROR: write data to file. result( %zu ) != fwrite( w_size( %zu ))\n\r", result, w_size);
                #endif
				return 1;
			}

            pos = pos + w_size;
		}
        fclose( fi );
	}
    //=====================================================================
    // CMP
    //=====================================================================
	else if (param_opt.op == OP_CMP){
		printf(PRINT_TIRE);
		printf(" COMPARE:\n\r");
		printf(PRINT_TIRE);

		// proverka nalichiya funct READ
		if (chip_array[ param_opt.chip_n ]->read == 0){
			printf("ERROR: function .READ not defined\n\r");
			return 1;
		}

		mem_cmp = (uint8*)malloc( chip_array[ param_opt.chip_n ]->density );
	    if (mem_cmp == NULL){
		    printf("ERROR: malloc( size[  %d  ]) for buffer compare.\n\r", chip_array[ param_opt.chip_n ]->density);
		    return 1;
	    }

		if (stat(param_opt.file_name, &stat_buf) == -1){
			printf("ERROR: fstat return error !\n\r");
			return 1;
		}

		// proverka diapazona offset+size <= density
		if (param_opt.offset + stat_buf.st_size > chip_array[ param_opt.chip_n ]->density ){
			printf("ERROR: file is BIG !!! file_size( %lld ) > chip_size(+offset %lld )\n\r", (long long)stat_buf.st_size, (long long)(chip_array[ param_opt.chip_n ]->density - param_opt.offset));
			return 1;
		}

		printf("File size = %lld bytes.\n\r", (long long)stat_buf.st_size);

		if (stat_buf.st_size == 0){
			printf("ERROR: file is empty.\n\r");
			return 1;
		}

		printf("Read data from file.\n\r");
        // read data from file
		fi = fopen(param_opt.file_name, "rb");
		if ( fi == NULL ){
			printf("ERROR: open file.\n\r");
			return 1;
		}

		pos = 0;
		while(pos != stat_buf.st_size){
            if (stat_buf.st_size - pos > FILE_RW_BLOCK_SIZE)
				r_size = FILE_RW_BLOCK_SIZE;
			else
                r_size = stat_buf.st_size - pos;

            result = fread( &mem[pos], 1, r_size, fi);
			if (result != r_size){
			    printf("ERROR: read data from file. result( %d ) != fread( r_size( %u ))\n\r", result, r_size);
				return 1;
			}

            pos = pos + r_size;
		}
        fclose( fi );

        #ifdef _WIN32
		printf("File load OK, len = %u.\n\r", pos);
        #else
		printf("File load OK, len = %zu.\n\r", pos);
        #endif

		printf(PRINT_TIRE);
		printf(" READ.\n\r");
		printf(PRINT_TIRE);

        status = chip_read(ftHandle, chip_array[ param_opt.chip_n ], param_opt.offset, mem_cmp, pos);
		if (status != FT_OK){
			printf("ERROR: IO.\n\r");
			return 1;
		}

		printf(PRINT_TIRE);
		printf(" COMPARE.\n\r");
		printf(PRINT_TIRE);

		for (size_t i=0; i<pos; i++){
			if (mem[i] != mem_cmp[i]){
                #ifdef _WIN32
				printf("COMPARE ERROR: ADDRES( 0x%x )   BUF_FILE( 0x%x ) <> BUF_FLASH( 0x%x ).\n\r", i, mem[i], mem_cmp[i]);
                #else
				printf("COMPARE ERROR: ADDRES( 0x%zx )   BUF_FILE( 0x%x ) <> BUF_FLASH( 0x%x ).\n\r", i, mem[i], mem_cmp[i]);
                #endif
				return 1;
			}
		}
        printf(" COMPARE OK.\n\r");
		printf(PRINT_TIRE);
	}
    //=====================================================================
	// ERASE
    //=====================================================================
	else if (param_opt.op == OP_ER){
		printf(PRINT_TIRE);
		printf(" ERASE:\n\r");
		printf(PRINT_TIRE);

		// proverka nalichiya funct ERASE
		if (chip_array[ param_opt.chip_n ]->erase == 0){
			printf("ERROR: function .ERASE not defined = 0\n\r");
			return 1;
		}

		// proverka nalichiya funct GET_STATUS
		if (chip_array[ param_opt.chip_n ]->get_status == 0){
			printf("ERROR: function .GEt_STATUS not defined = 0\n\r");
			return 1;
		}

		if (param_opt.size == 0){
    		printf("ERROR: size not defined = 0\n\r");
			return 1;
		}

		// proverka diapazona offset+size <= density
		if (param_opt.offset + param_opt.size > chip_array[ param_opt.chip_n ]->density ){
			printf("ERROR: size( %d ) is BIG !!! > chip_size(+offset %d )\n\r", param_opt.size, chip_array[ param_opt.chip_n ]->density - param_opt.offset);
			return 1;
		}

		status = chip_erase(ftHandle, chip_array[ param_opt.chip_n ], param_opt.offset, param_opt.size);
		if (status != FT_OK){
			printf("ERROR: Chip erase !\n\r");
			return 1;
		}
	}


	status = p_SPI_CloseChannel(ftHandle);
	return 0;
}

//=============================================================================
// chip read to array
//=============================================================================
int chip_read(FT_HANDLE ftHandle, const struct mem_chip_st *ch, const uint32_t adr_start, uint8_t *buf, const uint32_t len)
{
	FT_STATUS status;
	uint32 pos = 0; //position - skolko uhe prochitali
	uint32 size_for_read = 0;
	uint32 sizeTransfered = 0;

	printf("Chip[ %s ] READ, START = %d ( 0x%x )  LEN = %d ( 0x%x ).\r\n", ch->name, adr_start, adr_start, len, len);

	percent_calc(P_ST_INIT, "READ", len, pos);

	pos = 0;

	while (pos != len){
		if (len - pos >= ch->prog_buf_size)
	        size_for_read = ch->prog_buf_size;
		else
            size_for_read = len - pos;
		
		sizeTransfered = 0;
		status = ch->read( ftHandle, adr_start + pos, &buf[ pos ], size_for_read, &sizeTransfered);
		if (status != FT_OK){
			printf("ERROR: read data from chip, adr = 0x%x\n\r", pos);
		    return 1;
		}

		pos = pos + sizeTransfered;

		percent_calc(P_ST_CALC, "READ", len, pos);

	}//while (pos != len){

	percent_calc(P_ST_100, "READ", len, pos);

	printf(" Ready !\n\r");

	return FT_OK;
}

//=============================================================================
// chip read to array
//=============================================================================
int chip_write(FT_HANDLE ftHandle, const struct mem_chip_st *ch, const uint32 adr_start, uint8 *buf, const uint32 len)
{
	int res;
	FT_STATUS status;
	uint32 size_for_write = 0;
	uint32 sizeTransfered = 0;

	uint32 index = 0;

	printf("Chip[ %s ] WRITE, START = %d ( 0x%x) LEN = %d ( 0x%x).\r\n", ch->name, adr_start, adr_start, len, len);

	if (adr_start & (ch->prog_buf_size - 1)){
		printf("ERROR: addres start != n * PROG_BUF_SIZE. (offset = 0x%x)\n\r", adr_start & (ch->prog_buf_size - 1));
		return 1;
	}

	index = 0;
	percent_calc(P_ST_INIT, "WRITE", len, index);

	while (index != len){
		if (len - index >= ch->prog_buf_size){
            size_for_write = ch->prog_buf_size;
		}else{
            size_for_write = len - index;
		}

		status = ch->wren( ftHandle );
		if (status != FT_OK){
			printf("ERROR: wren, return =%d\n\r", status);
		    return 1;
		}

		status = ch->write( ftHandle, adr_start + index, &buf[ index ], size_for_write, &sizeTransfered);
		if (status != FT_OK){
			printf("ERROR: Write data to chip, adr=0x%x\n\r", adr_start + index);
		    return 1;
		}

		//CHIP READY ?
		while ((res = ch->get_status( ftHandle )) == 1);
		if (res == -1){
			printf("ERROR: get_status return error.\n\r");
			return 1;
		}

		if (sizeTransfered < size_for_write){
			printf("ERROR: Write( %d ) < PROG_BUF_SIZE( %d )\n\r", sizeTransfered, size_for_write);
			return 1;
		}
		index = index + sizeTransfered;

		status = ch->wrdi( ftHandle );
		if (status != FT_OK){
			printf("ERROR: wrdi, return =%d\n\r", status);
		    return 1;
		}

		percent_calc(P_ST_CALC, "WRITE", len, index);
	}//while (index != len){


    percent_calc(P_ST_100, "WRITE", len, index);

	printf(" Ready !\n\r");

	return FT_OK;
}
//=============================================================================
// chip erase
//=============================================================================
int chip_erase(FT_HANDLE ftHandle, const struct mem_chip_st *ch, const uint32 adr_start, const uint32 len)
{
	int res;
	FT_STATUS status;
	uint32 index = 0;
	uint32 pos = 0; //position - skolko uhe prochitali
	uint32 len_erase = 0;           // kolichestvo byte for dla stiraniya(bytes), s uchutom granich sectorov

	uint32 sectors_for_erase = 0;   // kolichestvo sectorov na stiranie
	uint32 adr_sec_start = 0;       // adres nachalnoy sectora stirabiya
	uint32 adr_sec_stop  = 0;       // adres konecnoy sectora stirabiya
	uint32 sec_erase_num_start = 0; // nomer sectora start
	uint32 sec_erase_num_stop = 0;  // nomer sectora stop

    // proveraem diapazon if adr start ne kraten nachalu stranicci to viravnivaem nachalniy adres na nachalo stranici
	// s - sector start
	// s-------s----------s--------s----------s---------s----------s----------s---------s  <- SECTORS
	//                 dddddddddddddddddddddddddddddddddddddddddddddddd                    <- DATA
	//         eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee             <- oblast dla stiraniya
	//

	sec_erase_num_start = (adr_start & (~(ch->sector_erase_size - 1))) / ch->sector_erase_size;          // nachalnaya stranicha dla stiraniya
	
	sec_erase_num_stop = ((adr_start + len) / ch->sector_erase_size); // konechnaya stranica dla stiraniya
	if ( (adr_start + len) % ch->sector_erase_size > 0) sec_erase_num_stop++; // ne kratnoe delenie, stiraem +1 sector
	sec_erase_num_stop--; // t.k. otchet idet s 0

	sectors_for_erase = sec_erase_num_stop - sec_erase_num_start + 1;     // kolichestvo sectorov na stiranie
	//sectors_for_erase = sectors_for_erase == 0 ? 1 : sectors_for_erase; // if sectors_for_erase == 0 then sectors_for_erase = 1

	adr_sec_start = sec_erase_num_start * ch->sector_erase_size;
    adr_sec_stop  = sec_erase_num_stop * ch->sector_erase_size;

	printf("Chip[ %s ] ERASE, START = 0x%x  LEN = 0x%x.\r\n", ch->name, adr_start, len);
	printf("Sectors for ERASE = %d\n\r", sectors_for_erase);
	printf("Sec START = %d ( 0x%x ), sec END = %d ( 0x%x ).\r\n", sec_erase_num_start, sec_erase_num_start, sec_erase_num_stop, sec_erase_num_stop);
	printf("Adr START = %d ( 0x%x ), adr END = %d ( 0x%x ).\r\n", adr_sec_start, adr_sec_start, adr_sec_stop, adr_sec_stop);

	len_erase = (sectors_for_erase) * ch->sector_erase_size; 
	pos = adr_sec_start;

	percent_calc(P_ST_INIT, "ERASE", len_erase, 0);

	index = 0;
	while (index != sectors_for_erase){
		
		// WR ENABLE
		status = ch->wren( ftHandle );
		if (status != FT_OK){
			printf("ERROR: WREN.\n\r");
			return 1;
		}
		
		// ERASE
		status = ch->erase( ftHandle, pos );
		if (status != FT_OK){
			printf("ERROR: ERASE adr = 0x%x\r\n", pos);
		    return 1;
		}

		//CHIP READY ?
		while ((res = ch->get_status( ftHandle )) == 1);
		if (res == -1){
			printf("ERROR: get_status return error.\n\r");
			return 1;
		}

		pos = pos + ch->sector_erase_size;
		index++;

		percent_calc(P_ST_CALC, "ERASE", len_erase, index * ch->sector_erase_size);
	}//while (pos != len){

	percent_calc(P_ST_100, "ERASE", len_erase, pos);
	printf(" Ready !\n\r");

	return FT_OK;
}
