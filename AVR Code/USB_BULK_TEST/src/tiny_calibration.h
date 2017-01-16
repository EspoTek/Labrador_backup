/*
 * tiny_calibration.h
 *
 * Created: 9/01/2017 4:21:09 PM
 *  Author: Esposch
 */ 


#ifndef TINY_CALIBRATION_H_
#define TINY_CALIBRATION_H_

#include <asf.h>
#include <stdio.h>

void tiny_calibration_init();
void tiny_calibration_first_sof();
void tiny_calibration_every_sof();
void tiny_calibration_safe_add(int rawValue);
int tiny_distance_from_centre(unsigned int point);
#endif /* TINY_CALIBRATION_H_ */