#ifndef GET_OPT_H_
#define GET_OPT_H_

#include "libMPSSE_spi.h"
 // for uint32

#define LEN_STRING_DEC_MAX   (10)
#define LEN_STRING_HEX_MAX   (10)


typedef enum {GTS_ERROR, GTS_DEC, GTS_HEX} type_string_e;
typedef enum { OP_NONE = 0, OP_RD, OP_WR, OP_CMP, OP_ER } operation_e;

typedef struct
{
	int chip_list;        // != 0 get chip list
	char * chip_name;     // nazvanie chipa
	int chip_n;           // nomer chip v spiske
	char * file_name;     // file name for I/O
	uint32 offset;        // adr-start
	uint32 size;          // len
	operation_e op;       // WR or RD or ER
	uint32 clk;           // clk for FT2232H
	uint32 channel;       // nomer kanala dla FTx232H

}param_opt_st;

int get_opt(const int argc, char** argv, param_opt_st *param_opt);
type_string_e get_type_string(const char * st);
int conv_to_uint32(const char * st, uint32_t *ui32);


#endif