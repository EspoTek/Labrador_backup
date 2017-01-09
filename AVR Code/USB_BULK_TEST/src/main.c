//Include the ASF Licence!

#include <stdio.h>
#include <asf.h>
#include "ui.h"
#include "globals.h"
#include "tiny_adc.h"
#include "tiny_dma.h"
#include "tiny_timer.h"
#include "tiny_dac.h"
#include "tiny_uart.h"
#include "tiny_dig.h"
#include "tiny_calibration.h"

volatile bool main_b_vendor_enable = false;

COMPILER_WORD_ALIGNED
volatile unsigned char isoBuf[BUFFER_SIZE];
COMPILER_WORD_ALIGNED
volatile unsigned char dacBuf_CH1[DACBUF_SIZE];// = {128,  134,  140,  146,  153,  159,  165,  171,  177,  182,  188,  194,  199,  204,  209,  214,  218,  223,  227,  230,  234,  237,  240,  243,  246,  248,  250,  252,  253,  254,  255,  255,  255,  255,  254,  253,  252,  251,  249,  247,  245,  242,  239,  236,  232,  229,  225,  220,  216,  211,  206,  201,  196,  191,  185,  180,  174,  168,  162,  156,  149,  143,  137,  131,  124,  118,  112,  106,   99,   93,   87,   81,   75,   70,   64,   59,   54,   49,   44,   39,   35,   30,   26,   23,   19,   16,   13,   10,    8,    6,    4,    3,    2,    1,    0,    0,    0,    0,    1,    2,    3,    5,    7,    9,   12,   15,   18,   21,   25,   28,   32,   37,   41,   46,   51,   56,   61,   67,   73,   78,   84,   90,   96,  102,  109,  115,  121,  127};
volatile unsigned char dacBuf_CH2[DACBUF_SIZE];

volatile unsigned char b1_state = 0;
volatile unsigned char b2_state = 0;
volatile unsigned char usb_state = 0;

volatile uint16_t dacBuf_len = 128;
volatile uint16_t auxDacBufLen = 128;
volatile unsigned char dummy = 0x55;
volatile unsigned char global_mode = 255;

volatile char PSU_target = 0;

volatile unsigned char test_byte = 123;

uint32_t debug_counter;

unsigned char tripleUsbSuccess = 0;

unsigned char firstFrame = 0;
unsigned char tcinit = 0;

int main(void){
	irq_initialize_vectors();
	cpu_irq_enable();
//	sysclk_init();

	
	//Set up 48MHz DFLL for USB.
	OSC.DFLLCTRL = OSC_RC32MCREF_USBSOF_gc;
	DFLLRC32M.CALB = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, USBRCOSC)); //THIS is the val for 48MHz.  RCOSC32M is for a 32MHz calibration.  That makes a lot of sense now...
	DFLLRC32M.COMP2 = 0xBB;
	DFLLRC32M.COMP1= 0x80;  //0xBB80 = 48,000.  
	DFLLRC32M.CTRL = 0x01; //Enable
		
	CCP = CCP_IOREG_gc;
	CLK.PSCTRL = CLK_PSADIV_2_gc | CLK_PSBCDIV_1_1_gc;  //All peripheral clocks = CLKsys / 2.
	//CLK.USBCTRL handled by udc
	OSC.CTRL = OSC_RC32MEN_bm | OSC_RC2MEN_bm;  //Enable 2MHz, 32MHz references.
	while(OSC.STATUS != (OSC_RC32MRDY_bm | OSC_RC2MRDY_bm)); //Wait for them all to be ready before continuing
	
	//4 step process from ASF manual.  Puts a 48MHz clock on the PLL output
	OSC.CTRL |= OSC_RC2MEN_bm;  //1. Enable reference clock source.
	OSC.PLLCTRL = OSC_PLLSRC_RC2M_gc | 24; //2. Set the multiplication factor and select the clock reference for the PLL.
	while(!(OSC.STATUS & OSC_RC2MRDY_bm)); //3. Wait until the clock reference source is stable.
	OSC.CTRL |= OSC_PLLEN_bm; //4. Enable the PLL
	
	//Move CPU + Peripherals to 48MHz clock.
	CCP = CCP_IOREG_gc;
	CLK.CTRL = CLK_SCLKSEL_RC32M_gc;
	
	board_init();
	udc_start();
	tiny_dac_setup();
	tiny_dma_setup();
	tiny_adc_setup(0, 0);
	tiny_adc_pid_setup();
	tiny_adc_ch1setup(12);
	tiny_timer_setup();
	tiny_uart_setup();
	tiny_spi_setup();
	tiny_dig_setup();
			
	//USARTC0.DATA = 0x55;
	//asm("nop");

	
	while (true) {
		debug_counter++;
		if(debug_counter > 100000000){
			debug_counter = 0;
		}
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
	//test_byte = ADCA.CH1.RESH;
	//DO NOTHING!
	}
}

//! Global variable to give and record information about setup request management
udd_ctrl_request_t udd_g_ctrlreq;

//CALLBACKS:
void main_suspend_action(void)
{
	return;
}

void main_resume_action(void)
{
	return;
}

void main_sof_action(void)
{
	if(firstFrame){

		tiny_calibration_init();
		firstFrame = 0;
		tcinit = 1;
	}
	else{
		if(tcinit){
			asm("nop");
		}
	}
	usb_state = !b1_state;
	return;
}

bool main_vendor_enable(void)
{
	main_b_vendor_enable = true;
	firstFrame = 1;
	udi_vendor_iso_in_run((uint8_t *)&isoBuf[0], PACKET_SIZE, iso_callback);
	udi_vendor_iso_in_run2((uint8_t *)&isoBuf[250], PACKET_SIZE, iso_callback2);
	udi_vendor_iso_in_run3((uint8_t *)&isoBuf[500], PACKET_SIZE, iso_callback3);
	return true;
}

void main_vendor_disable(void)
{
	main_b_vendor_enable = false;
}

bool main_setup_out_received(void)
{
	return 1;
}

bool main_setup_in_received(void)
{
	return true;
}

void iso_callback(udd_ep_status_t status, iram_size_t nb_transfered, udd_ep_id_t ep){
	udi_vendor_iso_in_run((uint8_t *)&isoBuf[usb_state * PACKET_SIZE], 250, iso_callback);
	//if((int8_t) USB.FIFORP > -16) udi_vendor_iso_in_run((uint8_t *)&isoBuf[0], PACKET_SIZE, iso_callback);
	return;
}

void iso_callback2(udd_ep_status_t status, iram_size_t nb_transfered, udd_ep_id_t ep){
	udi_vendor_iso_in_run2((uint8_t *)&isoBuf[usb_state * PACKET_SIZE + 250], 250, iso_callback2);
	//if((int8_t) USB.FIFORP > -16) udi_vendor_iso_in_run((uint8_t *)&isoBuf[0], PACKET_SIZE, iso_callback);
	return;
}

void iso_callback3(udd_ep_status_t status, iram_size_t nb_transfered, udd_ep_id_t ep){
	udi_vendor_iso_in_run3((uint8_t *)&isoBuf[usb_state * PACKET_SIZE + 500], 250, iso_callback3);
	//if((int8_t) USB.FIFORP > -16) udi_vendor_iso_in_run((uint8_t *)&isoBuf[0], PACKET_SIZE, iso_callback);
	return;
}


