#include <stdio.h>
#include <stdint.h>
#include "percent_calc.h"
//=============================================================================
// Caclul % i print to console
//
// st  - string print pere %
// len - data len = 100 %
// pos - tekucha position
//=============================================================================
static uint32_t per1  = 0; // 1 %
static uint32_t per_c = 0;
static uint32_t per_p = 0;

void percent_calc(procent_stage_st p_sta, char * st, const uint32_t len, const uint32_t pos )
{
	
    switch( p_sta ){
    case P_ST_INIT:
	{
        per_p = 0;
        per1 = len / 100;         // calc 1 %
        if (per1 == 0 ) per1 = 1;
        printf("%s: 0 %% \r", st);
        break;
    }

    case P_ST_CALC:
	{
        per_c = pos / per1;
	    if (per_c >= per_p){
	        per_p = per_c;
	        printf("%s: %d %%   \r", st, per_p);
	    }
        break;
    }

    case P_ST_100:
	{
        printf("%s: 100 %%   \n\r", st);
        break;
    }

    default:
    break;
    }//switch(p_sta){

}
