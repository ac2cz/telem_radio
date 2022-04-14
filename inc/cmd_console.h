/*
 * cmd_console.h
 *
 *  Created on: Apr 11, 2022
 *      Author: g0kla
 */

#ifndef CMD_CONSOLE_H_
#define CMD_CONSOLE_H_

/* Start the command console and wait for input from the user.  This will block and continue to run until
 * either the user enters the exit command, jackd exits and closes the program or an interrupt signal
 * is received */
int start_cmd_console();

#endif /* CMD_CONSOLE_H_ */
