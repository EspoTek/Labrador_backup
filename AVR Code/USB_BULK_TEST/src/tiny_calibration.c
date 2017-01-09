/*
 * tiny_calibration.c
 *
 * Created: 9/01/2017 4:20:59 PM
 *  Author: Esposch
 */ 

#include "tiny_calibration.h"
#include "globals.h"

tiny_calibration_init(){
	PR.PRPE &= 0b11111110;
	TC_CALI.PER = 24000;
	TC_CALI.CNT = 12000;
	TC_CALI.CTRLA = TC_CLKSEL_DIV1_gc;
	return;
}