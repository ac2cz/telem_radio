/*
 * serial.c
 *
 *  Created on: Oct 2, 2022
 *      Author: g0kla
 */


#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Forward functions */
void rts_on(int fd);
void rts_off(int fd);

//static struct termios oldterminfo;

void closeserial(int fd) {
    if (close(fd) < 0)
        perror("closeserial()");
}

int openserial(char *devicename)
{
    int fd;
   // struct termios attr;

    if ((fd = open(devicename, O_RDWR)) == -1) {
        perror("openserial(): open()");
        return 0;
    }

    return fd;
}

void set_rts(int fd, int state) {
	if (state)
		rts_on(fd);
	else
		rts_off(fd);
}

void rts_on(int fd) {
	int attr;
	ioctl (fd, TIOCMGET, &attr);
	attr |= TIOCM_RTS;
	ioctl (fd, TIOCMSET, &attr);
}

void rts_off(int fd) {
	int attr;
	ioctl (fd, TIOCMGET, &attr);
	attr &= ~TIOCM_RTS;
	ioctl (fd, TIOCMSET, &attr);
}

