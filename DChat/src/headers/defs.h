/*
 * defs.h
 *
 *  Created on: Mar 31, 2015
 *      Author: arpit
 */

#ifndef SRC_HEADERS_DEFS_H_
#define SRC_HEADERS_DEFS_H_

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <pthread.h>
#include <vector>
#include <cctype>
#include <algorithm>
#include <map>
#include <list>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <queue>
#include <sys/time.h>
#include <ifaddrs.h>
#include <netinet/in.h>

#define MESSAGE_TYPE_CHAT 1
#define MESSAGE_TYPE_JOIN 2
#define MESSAGE_TYPE_DELETE 3
#define MESSAGE_TYPE_UPDATE 4
#define MESSAGE_TYPE_CHECK 5
#define MESSAGE_TYPE_PROPOSED 6
#define MESSAGE_TYPE_NOTICE 7
#define MESSAGE_TYPE_REMOVE_USER 8

#define DEFAULT_INTERFACE "em1"

#define MESSAGE_SIZE 2048
#define MAX_USERS 25
#define NUM_THREADS 10
#define IP_BUFSIZE 16
#define PORT_BUFSIZE 5
#define USERNAME_BUFSIZE 32
#define RXBYTE_BUFSIZE 32
#define BASE_PORT 8980
#define USAGE "Usage:\nTo start a new chat: dchat <NAME> \nTo join an existing chat: dchat <NAME> <IP>:<PORT>\n<NAME>: The name to be visible to other users\n<IP>:<PORT> :- IP Address and Port of the client to send join request to "

#define BEAT "BEAT"
#define RESPONSE_BEAT "ALIVE"

//Function declarations
void addUser(char ipaddress[],int portNum,char username[]);

void addSocket(char ipaddress[], int portNum);

void addUserlist(char ipaddress[],int portNum, char username[],char rxsize[]);

int populatelistofUsers(char *users);

void sendlist(char *msg);

void populatesocketClient(char userList[],int numUser);

void incrementCount();

void initializeCount();


#endif /* SRC_HEADERS_DEFS_H_ */
