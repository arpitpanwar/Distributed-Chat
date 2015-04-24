#include "headers/udpserver.h"
#include "headers/defs.h"

udp_Server::udp_Server(const char *ipAddr, int port ){
	f_port = port;
	f_addr = inet_addr(ipAddr);
	f_socket = socket(AF_INET, SOCK_DGRAM , 0 );
	if(f_socket < 0 ){
		perror("Error while creating socket ");
	}

	f_addrserver.sin_addr.s_addr = htons(INADDR_ANY);
	f_addrserver.sin_family = AF_INET;
	f_addrserver.sin_port = htons(port);
	if(bind(f_socket,(struct sockaddr *) &f_addrserver, sizeof(f_addrserver))< 0 ) {
		perror("Error while binding the socket \n");

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
int udp_Server::get_message(struct sockaddr_in client,char *msg, size_t max_size){
	int addr_len = sizeof(struct sockaddr);
	return ::recvfrom(f_socket, msg,max_size, 0, (struct sockaddr *) &client, (socklen_t *)&addr_len);
}

int udp_Server::send_message(struct sockaddr_in client,char *msg, size_t max_size ){
	return::sendto(f_socket,msg,max_size,0,(struct sockaddr *) &client,(socklen_t)sizeof(struct sockaddr));
}

