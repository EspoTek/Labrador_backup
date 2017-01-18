/*
 * tiny_calibration.c
 *
 * Created: 9/01/2017 4:20:59 PM
 *  Author: Esposch
 */ 

#include "tiny_calibration.h"
#include "globals.h"
#include "tiny_adc.h"

void tiny_calibration_init(){
		//Set up 48MHz DFLL for USB.
		OSC.DFLLCTRL = OSC_RC32MCREF_USBSOF_gc;
		DFLLRC32M.CALB = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, USBRCOSC)); //THIS is the val for 48MHz.  RCOSC32M is for a 32MHz calibration.  That makes a lot of sense now...
		DFLLRC32M.COMP2 = 0xBB;
		DFLLRC32M.COMP1= 0x80;  //0xBB80 = 48,000.
		DFLLRC32M.CTRL = 0x01; //Enable
		
		//Turn on the 48MHz clock and scale it down to 24MHz
		CCP = CCP_IOREG_gc;
		CLK.PSCTRL = CLK_PSADIV_2_gc | CLK_PSBCDIV_1_1_gc;  //All peripheral clocks = CLKsys / 2.
		//CLK.USBCTRL handled by udc
		OSC.CTRL = OSC_RC32MEN_bm | OSC_RC2MEN_bm;  //Enable 32MHz reference.  Keep 2MHz on.
		while(OSC.STATUS != (OSC_RC32MRDY_bm | OSC_RC2MRDY_bm)); //Wait for it to be ready before continuing
		
		//4 step process from ASF manual.  Puts a 48MHz clock on the PLL output
		OSC.CTRL |= OSC_RC2MEN_bm;  //1. Enable reference clock source.
		OSC.PLLCTRL = OSC_PLLSRC_RC2M_gc | 24; //2. Set the multiplication factor and select the clock reference for the PLL.
		while(!(OSC.STATUS & OSC_RC2MRDY_bm)); //3. Wait until the clock reference source is stable.
		OSC.CTRL |= OSC_PLLEN_bm; //4. Enable the PLL
		
		//Move CPU + Peripherals to 48MHz PLLL clock.
		while(!(OSC.STATUS & OSC_PLLRDY_bm));
		CCP = CCP_IOREG_gc;
		CLK.CTRL = CLK_SCLKSEL_PLL_gc;
		
		//DFLLRC2M.CALB -= 1;
		//DFLLRC2M.CALA -= 21;
		return;
}

tiny_calibration_first_sof(){
		PR.PRPE &= 0b11111110;
		TC_CALI.PER = 24000;
		TC_CALI.CNT = 12000;
		TC_CALI.CTRLA = TC_CLKSEL_DIV1_gc;
		return;
}

unsigned char deadTime = 0;
volatile unsigned long outOfRange = 0;

volatile unsigned char cali_value_negative_gradient;
volatile unsigned char cali_value_positive_gradient;
volatile unsigned char warmup = 10;
void tiny_calibration_maintain(){
	unsigned int cnt = TC_CALI.CNT;
	
	if(cnt > 12000){
		DFLLRC2M.CALA = cali_value_negative_gradient;
	}
	if(cnt < 12000){
		DFLLRC2M.CALA = cali_value_positive_gradient;
	}
	if(warmup){
		warmup--;  //There's a warmup period in case tiny_calibration_find_values returns outside range; it won't record out of range until this period is over.
	}
	else if((cnt<11000) || (cnt>13000)){
		//This is an untested, last-ditch effort to hopefully prevent runaway due to drift over time.
		calibration_values_found = 0x00;
		outOfRange++;
		warmup = 6;
	}
	
	return;
}

volatile unsigned int calTemp;
void tiny_calibration_safe_add(int rawValue){
	unsigned int addValue;
	unsigned char subtract;
	if(rawValue == 0){
		return;
	}
	if(rawValue > 0){
		addValue = (unsigned int) rawValue;
		subtract = 0;
	}
	if(rawValue < 0){
		rawValue = -rawValue;
		addValue = rawValue;
		subtract = 1;
	}
	calTemp = DFLLRC2M.CALB;
	calTemp = calTemp << 7;
	calTemp += DFLLRC2M.CALA;
	asm("nop");
	if(calTemp < addValue){
		calTemp=0;
		return;
	}
	if((calTemp + addValue) > 0x1fff){
		calTemp = 0x1fff;
		return;
	}
	if(subtract){
		calTemp -= addValue;
	}
	else{
		calTemp += addValue;
	}
	DFLLRC2M.CALA = calTemp & 0x7f;
	//DFLLRC2M.CALB = calTemp >> 7;	
	}

int tiny_distance_from_centre(unsigned int point){
	int midVal = point-12000;
	return midVal < 0 ? -midVal : midVal;
}

volatile unsigned char calibration_values_found = 0x00;
volatile unsigned int last_val = 12000;
volatile int gradient;
volatile unsigned int calChange;
#define NUM_INAROW 12
volatile unsigned char inarow = NUM_INAROW;

void tiny_calibration_find_values(){
	unsigned int cnt = TC_CALI.CNT;
	gradient = cnt - last_val;
	
	//Find the negative value first.
	if(calibration_values_found == 0x00){
		if((gradient < -50) && (gradient > -150)){
			if(inarow){
				inarow--;
				}else{
				cali_value_negative_gradient = DFLLRC2M.CALA;
				calibration_values_found = 0x01;
				inarow = NUM_INAROW;
			}
		}
		else{
			inarow = NUM_INAROW;
			calChange = gradient < -150 ? 1 : -1;
			calChange -= gradient / 48;
			tiny_calibration_safe_add(calChange);
		}
	}
	
	//Search for the positive gradient
	if(calibration_values_found == 0x01){
		if(gradient > 50){
			if(inarow){
				inarow--;
				} else{
				cali_value_positive_gradient = DFLLRC2M.CALA;
				calibration_values_found = 0x03;
			}
		}
		else tiny_calibration_safe_add((gradient > 150 ? -1 : 1));
	}
	last_val = cnt;
}