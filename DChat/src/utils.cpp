#include <stdio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include<iostream>
#include "headers/udpserver.h"
#include "headers/defs.h"
#include "headers/chatstructures.h"
#include <stdlib.h>
#include <unistd.h>
#include <sstream>


using namespace std;


void populateLeader(LEADER *lead,char ip[],int portNum,char username[]){
	strcpy(lead->sIpAddress,ip);
	lead->sPort = portNum;
	strcpy(lead->sName,username);
}

string findip(){
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            string name(ifa->ifa_name);
            if(name.compare(DEFAULT_INTERFACE)==0){
            	string address(addressBuffer);
            	return address;
            }
            //printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return NULL;

}

void printAllUsers(map<string,string> clientMap ,bool isLeader){
	map<string,string>::iterator pos;
		for(pos = clientMap.begin(); pos != clientMap.end();++pos ){
			cout <<pos->first <<" "<<pos->second;
			if(isLeader)
				cout<<" Leader";
			cout << "\n";
		}
}


bool isPortAvailable(int port){
		bool available = false;
		char* hostname ="localhost";
		struct sockaddr_in client;
		struct hostent *server;
	    int sock;

	    server = gethostbyname(hostname);

	    client.sin_family = AF_INET;
	    client.sin_port = htons(port);
	    bcopy((char *)server->h_addr,(char *)&client.sin_addr.s_addr, server->h_length);


	    sock = (int) socket(AF_INET, SOCK_DGRAM, 0);

	    int result = bind(sock, (struct sockaddr *) &client,sizeof(client));

	    if(result<0){
	    	available = false;
	    }else{
	    	available = true;
	    }

	    close(sock);

	    return available;
}

int getOpenPort(){

		int port = BASE_PORT;

		if(isPortAvailable(port) & isPortAvailable(port+1) & isPortAvailable(port+2)){

			return port;
		}else{
			port = port+10;
			while(!isPortAvailable(port) & !isPortAvailable(port+1) & !isPortAvailable(port+2)){
				port = port+10;
			}
		}
		return port;
}

vector<string> split(string s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> elems;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

