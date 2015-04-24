#include "headers/chatstructures.h"
#include "headers/defs.h"

chat_node::chat_node(char userName[],int entry,char ipaddr[], int port ){

	strcpy(sUserName,userName);
	strcpy(ipAddress,ipaddr);
	portNum = port;
	entryNum  = entry;

}

chat_node::~chat_node(){

}
