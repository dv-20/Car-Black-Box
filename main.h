#ifndef MAIN_H
#include <stdint.h>
#define MAX_GEAR 6
#define SPEED_ADC_CHANNEL 0x04
#define MODE_MENU         MK_SW4

#define GEAR_UP             MK_SW1
#define GEAR_DOWN           MK_SW2
#define COLLISION           MK_SW3

uint16_t get_speed();
unsigned char get_gear_pos();

#endif