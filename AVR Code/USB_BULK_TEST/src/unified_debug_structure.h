/*
 * unified_debug_structure.h
 *
 * Created: 1/02/2017 9:38:31 AM
 *  Author: Esposch
 */


#ifndef UNIFIED_DEBUG_STRUCTURE_H_
#define UNIFIED_DEBUG_STRUCTURE_H_

#include <stdint.h>

//EVERYTHING MUST BE SENT ONE BYTE AT A TIME, HIGH AND LOW BYTES SEPARATE, IN ORDER TO AVOID ISSUES WITH ENDIANNESS.
typedef struct uds{
    volatile char header[9];
    volatile uint8_t trfcntL0;
    volatile uint8_t trfcntH0;
	volatile uint8_t medianTrfcntL;
	volatile uint8_t medianTrfcntH;
	volatile uint8_t calValNeg;
	volatile uint8_t calValPos;
	volatile uint8_t CALA;
	volatile uint8_t CALB;
} unified_debug;

#endif /* UNIFIED_DEBUG_STRUCTURE_H_ */
