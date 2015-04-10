#include "headers/chatstructures.h"
#include<stdlib.h>
#include<string.h>
chat_node::chat_node(char userName[],int entry,char ipaddr[], int port ){

	strcpy(sUserName,userName);
	strcpy(ipAddress,ipaddr);
	portNum = port;
	bIsLeader = true;
	entryNum  = entry;

}

chat_node::~chat_node(){

}
