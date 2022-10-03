/*
 * serial.h
 *
 *  Created on: Oct 2, 2022
 *      Author: g0kla
 */

#ifndef SERIAL_H_
#define SERIAL_H_

void closeserial(int fd);
int openserial(char *devicename);
int setRTS(int fd, int level);

#endif /* SERIAL_H_ */
