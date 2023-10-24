/*
 * gpio_interface.h
 *
 *  Created on: Apr 13, 2022
 *      Author: g0kla
 *
 *
 *
 * Running pinout on the raspberry pi must match the below
 *

J8:
   3V3  (1) (2)  5V
 GPIO2  (3) (4)  5V
 GPIO3  (5) (6)  GND
 GPIO4  (7) (8)  GPIO14
   GND  (9) (10) GPIO15
GPIO17 (11) (12) GPIO18
GPIO27 (13) (14) GND
GPIO22 (15) (16) GPIO23
   3V3 (17) (18) GPIO24
GPIO10 (19) (20) GND
 GPIO9 (21) (22) GPIO25
GPIO11 (23) (24) GPIO8
   GND (25) (26) GPIO7
 GPIO0 (27) (28) GPIO1
 GPIO5 (29) (30) GND
 GPIO6 (31) (32) GPIO12
GPIO13 (33) (34) GND
GPIO19 (35) (36) GPIO16
GPIO26 (37) (38) GPIO20
   GND (39) (40) GPIO21

POE:
TR01 (1) (2) TR00
TR03 (3) (4) TR02

For further information, please refer to https://pinout.xyz/
 *
 *
 *
 */

#ifndef GPIO_INTERFACE_H_
#define GPIO_INTERFACE_H_

#ifdef RASPBERRY_PI
#include <bcm2835.h>

#define GPIO_PTT RPI_BPLUS_GPIO_J8_07 // GPIO4
uint16_t clk_div = BCM2835_I2C_CLOCK_DIVIDER_148;

#endif /* RASPBERRY_PI */

#ifndef RASPBERRY_PI
/* Define these so we can use them in the code.  But they are defined in the lib
 * when on the PI */
#define HIGH 1
#define LOW 0

#endif /* NOT RASPBERRY_PI */

int gpio_init();
int gpio_set_ptt(int state);

#endif /* GPIO_INTERFACE_H_ */
