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

#define MESSAGE_TYPE_CHAT "Chat:"

#define MESSAGE_TYPE_ELECTION "Election:"
#define MESSAGE_TYPE_USERLIST "UserList:"

#define MESSAGE_TYPE_STATUS "Status:"
#define MESSAGE_TYPE_STATUS_JOIN "Join"
#define MESSAGE_TYPE_STATUS_LEAVE "Leave"
#define MESSAGE_TYPE_STATUS_ALIVE "Alive"
#define MESSAGE_TYPE_STATUS_ISALIVE "IsAlive"

#define DEFAULT_INTERFACE "eth0"

#define MESSAGE_SIZE 2048
#define MAX_USERS 25

#endif /* SRC_HEADERS_DEFS_H_ */
