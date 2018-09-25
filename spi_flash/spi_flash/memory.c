#include <stdio.h>
#include "memory.h"
#include "s25fl512s.h"
#include "m25p64.h"
#include "m25p128.h"

//=============================================================================
// global array structure pointer to chip
//=============================================================================

const struct mem_chip_st *chip_array[] = {
    &chip_s25fl512s,
	&chip_m25p64,
	&chip_m25p128,

};

//=============================================================================
// kolichestvo mikroshem v massive
//=============================================================================
const unsigned int CHIP_QUANTITY = sizeof( chip_array ) / sizeof( *chip_array );

