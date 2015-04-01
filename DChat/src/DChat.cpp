
// Author      : Anand Sriramulu
// Version     :
// Copyright   : Your copyright notice
// Description : Main file of the chat service.
//============================================================================

#include <iostream>
#include<cstdlib>
#include<sstream>
#include<arpa/inet.h>
#include<sys/unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<stdio.h>
#include <string.h>

#include "headers/udpServer.h"
#include "headers/defs.h"

using namespace std;

int main(int argc, char *argv[]) {
	string ipaddress;

	ipaddress = findip();

	if(argc < 4){
		cout << argv[1] << " started a new chat,listening on "<<ipaddress<<":"<<argv[2];
	}

	return 0;
}
