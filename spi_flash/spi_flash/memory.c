#include <stdio.h>
#include "memory.h"
#include "s25fl512s.h"
#include "m25p16.h"
#include "m25p64.h"
#include "m25p128.h"
#include "w25q32.h"

//=============================================================================
// global array structure pointer to chip
//=============================================================================

const struct mem_chip_st *chip_array[] = {
	&chip_m25p16,
	&chip_m25p64,
	&chip_m25p128,
    &chip_s25fl512s,
	&chip_w25q32,

};

//=============================================================================
// kolichestvo mikroshem v massive
//=============================================================================
const unsigned int CHIP_QUANTITY = sizeof( chip_array ) / sizeof( *chip_array );

