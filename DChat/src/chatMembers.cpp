#include "headers/chatstructures.h"
#include<stdlib.h>
#include<string.h>
chat_node::chat_node(char userName[],int entry,char ipaddr[], int port , long lastSeq ){

	strcpy(sUserName,userName);
	strcpy(ipAddress,ipaddr);
	portNum = port;
	entryNum  = entry;
	lastSeqNum = lastSeq;

}

chat_node::~chat_node(){

}
