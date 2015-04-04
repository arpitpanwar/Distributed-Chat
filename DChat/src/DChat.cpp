
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
#include<sstream>
#include<pthread.h>

#include "headers/udpserver.h"
#include "headers/defs.h"
#include "headers/chatstructures.h"


void printAllUsers(map<string,string> clientMap ,bool isLeader);
using namespace std;

udp_Server* curServer;
chat_node* curNode;

void *recvMsg(void *id){
	struct sockaddr_in client;
	int bytes_recv;
	int addr_len = sizeof(struct sockaddr);
	string chatmsg;

	while(1){
		chatmsg.resize(2048);
		cout << "Waiting for message \n";

		bytes_recv = recvfrom(curServer->get_socket(),&chatmsg[0],2048,0,
				(struct sockaddr *)&client,(socklen_t*)&addr_len);

		if(bytes_recv < 0){
			perror("Error while receiving message \n");
			continue;
		}
		cout << "message received : " << chatmsg;



	}

	/*return;*/
}
void *sendMsg(void *id){
	struct sockaddr_in client;
	int ret;
	string chatmsg;
	client.sin_addr.s_addr = htons(INADDR_ANY);
	client.sin_family = AF_INET;
	client.sin_port = htons(4040);
	while(1){
		chatmsg.resize(2048);
		chatmsg = "Hello World \n";
		sleep(1);
		ret = sendto(curServer->get_socket(),chatmsg.c_str(),2048,0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
		if ( ret < 0){
			perror("error while sending the message \n");

			exit(1);
		}

		cout << "Message sent\n";
	}
}

int main(int argc, char *argv[]) {

	int i ,ret;
	string ipaddr_port;
	int isSequencer;
	string chatmsg;
	string username;

	string ipaddress;
	pthread_t threads[NUM_THREADS];
	ipaddress = findip();


	username = argv[1];
	isSequencer = *argv[3] - '0';
	// Getting the ip address in the format ip:port
	ipaddr_port = ipaddress+ ":" + argv[2];



	curNode = new chat_node(argv[1],1,ipaddress,argv[2]);



//TODO: Currently passing an argument say 1 or 0 indicating if the current program
// is starting a chat or joining a chat 1 : starting 0 : joining
	if(isSequencer){
		curNode->mClientmap[username] = ipaddr_port;
	}

	curServer = new udp_Server(ipaddr_port.c_str(),4040);
	if(curServer->get_socket() < -1){
		cout << "Client socket creation failed \n";
		exit(1);
	}
	if(isSequencer){
		cout << username << " started a new chat,listening on " << ipaddr_port;
		cout << "\nSucceeded,current users:\n";
		//Print all the users
		printAllUsers(curNode->mClientmap,curNode->bIsLeader);
		cout << "Waiting for others to join ... \n";
	}

		ret = pthread_create(&threads[0],NULL,recvMsg,NULL);
		if (ret){
			perror("Error While creating thread 1 \n");
			exit(1);
		}

		ret = pthread_create(&threads[0],NULL,sendMsg,NULL);
		if (ret){
			perror("Error While creating thread 1 \n");
			exit(1);
		}

	while(1){

	}

	return 0;
}
