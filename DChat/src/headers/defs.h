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
#include <string.h>
#include <sys/time.h>

#define MESSAGE_TYPE_CHAT 1

#define MESSAGE_TYPE_ELECTION 2
#define MESSAGE_TYPE_USERLIST 3

#define MESSAGE_TYPE_STATUS 4
#define MESSAGE_TYPE_STATUS_JOIN 5
#define MESSAGE_TYPE_STATUS_LEAVE 6
#define MESSAGE_TYPE_STATUS_ALIVE 7
#define MESSAGE_TYPE_STATUS_ISALIVE 8
#define MESSAGE_TYPE_CHAT_NOSEQ 9
#define MESSAGE_TYPE_UPDATE 10
#define DEFAULT_INTERFACE "eth0"

#define ELECTION_HAPPENING 11
#define NORMAL_OPERATION 12
#define MESSAGE_TYPE_LEADER 13
#define MESSAGE_TYPE_REMOVE_USER 14

#define MESSAGE_SIZE 2048
#define MAX_USERS 25
#define NUM_THREADS 10
#define IP_BUFSIZE 16
#define PORT_BUFSIZE 5
#define USERNAME_BUFSIZE 32
#define RXBYTE_BUFSIZE 32
#define BASE_PORT 8980
#define USAGE "Usage:\nTo start a new chat: dchat <NAME> \nTo join an existing chat: dchat <NAME> <IP>:<PORT>\n<NAME>: The name to be visible to other users\n<IP>:<PORT> :- IP Address and Port of the client to send join request to "

#define INVALID_MESSAGE_SIZE "Only Chat messages less that 2048 characters supported"

#define BEAT "BEAT"
#define RESPONSE_BEAT "ALIVE"

#endif /* SRC_HEADERS_DEFS_H_ */
