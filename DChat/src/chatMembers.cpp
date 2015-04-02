#include "headers/chatMembers.h"

chat_node::chat_node(string userName,int entry,string ipaddr, string port ){
	sUserName = userName;

	entryNum = entry;
	if(entryNum){
		bIsLeader = 1;
		lead.sIpAddress = ipaddr;
		lead.sName = userName;
		lead.sPort = port;
		lSequencenums = 0;
	}
	else
		bIsLeader = 0;

}


chat_node::~chat_node(){

}
