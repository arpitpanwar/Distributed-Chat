#include "headers/udpserver.h"

#include<string.h>
#include<stdio.h>
#include<arpa/inet.h>
#include<unistd.h>

udp_Server::udp_Server(char *ipAddr, int port ){
	f_port = port;
	f_addr = inet_addr(ipAddr);
	f_socket = socket(AF_INET, SOCK_DGRAM , 0 );
	if(f_socket < 0 ){
	//	throw chatServer_runtime_error("socket creation failed \n");
	}

		f_addrserver.sin_addr.s_addr = htons(INADDR_ANY);
		f_addrserver.sin_family = AF_INET;
		f_addrserver.sin_port = htons(port);
		if(bind(f_socket,(struct sockaddr *) &f_addrserver, sizeof(f_addrserver))< 0 ) {
		//	throw chatServer_runtime_error("Binding failed \n");

		}


}

udp_Server::~udp_Server(){
	close(f_socket);
}


int udp_Server::get_socket()const {
	return f_socket;
}
int udp_Server::get_portNum()const {
	return f_port;
}

std::string udp_Server::get_addr() const {
	return f_addr;
}

int udp_Server::get_message(char *msg, size_t max_size , int clientid){
	int len = sizeof(f_user[clientid].f_addrclient);
	return ::recvfrom(f_socket, msg,max_size, 0, (struct sockaddr *) &f_user[clientid].f_addrclient, (socklen_t *)&len);
}

int udp_Server::send_message(char *msg, size_t max_size ,int clientid){
	return::sendto(f_socket,msg,max_size,0,(struct sockaddr *) &f_user[clientid].f_addrclient,(socklen_t)sizeof(struct sockaddr));
}

