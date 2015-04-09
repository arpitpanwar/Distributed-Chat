/*
 * defs.h
 *
 *  Created on: Mar 31, 2015
 *      Author: arpit
 */

#ifndef SRC_HEADERS_DEFS_H_
#define SRC_HEADERS_DEFS_H_

#include<map>
#include<list>
#include<algorithm>
#include<stdlib.h>
#include<string>

#define MESSAGE_TYPE_CHAT 1

#define MESSAGE_TYPE_ELECTION 2
#define MESSAGE_TYPE_USERLIST 3

#define MESSAGE_TYPE_STATUS 4
#define MESSAGE_TYPE_STATUS_JOIN 5
#define MESSAGE_TYPE_STATUS_LEAVE 6
#define MESSAGE_TYPE_STATUS_ALIVE 7
#define MESSAGE_TYPE_STATUS_ISALIVE 8
#define MESSAGE_TYPE_CHAT_NOSEQ 9
#define DEFAULT_INTERFACE "wlan0"

#define MESSAGE_SIZE 2048
#define MAX_USERS 25
#define NUM_THREADS 5
#define IP_BUFSIZE 16
#define PORT_BUFSIZE 4
#define USERNAME_BUFSIZE 32

#endif /* SRC_HEADERS_DEFS_H_ */
