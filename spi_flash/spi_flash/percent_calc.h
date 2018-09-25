#ifndef PERCENT_CALC_H_
#define PERCENT_CALC_H_

typedef enum { P_ST_INIT, P_ST_CALC, P_ST_100 } procent_stage_st;

void percent_calc(procent_stage_st p_sta, char * st, const uint32_t len, const uint32_t pos );

#endif