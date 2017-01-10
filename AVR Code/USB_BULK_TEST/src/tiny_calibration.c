/*
 * tiny_calibration.c
 *
 * Created: 9/01/2017 4:20:59 PM
 *  Author: Esposch
 */ 

#include "tiny_calibration.h"
#include "globals.h"
#include "tiny_adc.h"

tiny_calibration_init(){
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

volatile unsigned int last_val = 12000;

void tiny_calibration_every_sof(){
	unsigned int cnt = TC_CALI.CNT;
	int gradient = cnt - last_val;
	
	//Unsafe increment/decrement!!
	if(cnt < 12000) tiny_calibration_safe_add(-1);
	if(cnt > 12000) tiny_calibration_safe_add(1);
	tiny_calibration_safe_add(-gradient/64);
	//tiny_calibration_safe_add(-15);
	
	last_val = cnt;
	//DFLLRC2M.CALB += ((TC_CALI.CNT < 12000) ? 1 : -1 );
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
	DFLLRC2M.CALB = calTemp >> 7;	
}