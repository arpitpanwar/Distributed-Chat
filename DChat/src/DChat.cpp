
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
#include<vector>
#include<cctype>
#include<algorithm>
#include "headers/udpserver.h"
#include "headers/defs.h"
#include "headers/chatstructures.h"

//#define DEBUG
#define RETURN



using namespace std;

udp_Server* curServer;
chat_node* curNode;




void *printConsole(void *id){

	string msg;
	while(true){
		if(!curNode->printQueue.empty()){
#ifdef DEBUG
			cout << "Print thread entered \n";
#endif
			//msg = curNode->printQueue.front();
			msg =curNode->printQueue.pop();
			cout <<"\nPrint on Console :: " +msg + "\n";
		}
	}

}

void *recvMsg(void *id){

	struct sockaddr_in client;
	int bytes_recv;
	int addr_len = sizeof(struct sockaddr);


	while(true){
		MESSAGE msg;

		int seqNum = 0 ;

#ifdef DEBUG
		cout << "Receive Message thread entered \n";
#endif
		bytes_recv = recvfrom(curServer->get_socket(),&msg,sizeof(MESSAGE),0,
				(struct sockaddr *)&client,(socklen_t*)&addr_len);

		if(bytes_recv < 0){
			perror("Error while receiving message \n");
			continue;
		}


		seqNum = msg.lSequenceNums;
		cout << "\nMessage being received :: " ;
		cout << msg.sContent;

		if(seqNum!=0){
			curNode->holdbackQueue.push(msg);
		}
		else{
			curNode->consoleQueue.push(msg);
		}

	}
}
void *sendMsg(void *id){
	struct sockaddr_in client;
	int ret;
	int seqNum = 0;
	while(true){

			MESSAGE msgTosend;
			if(!curNode->sendQueue.empty()){
				seqNum ++;
				msgTosend = curNode->sendQueue.pop();
				msgTosend.lSequenceNums = seqNum;

#ifdef DEBUG
			cout << "Send Message: Send Queue not empty\n";
			cout << curSentmsg.message;
#endif

				for (std::list<sockaddr_in>::const_iterator iterator = curNode->listOfUsers.begin(), end = curNode->listOfUsers.end(); iterator != end; ++iterator) {
					client = *iterator;

					ret = sendto(curServer->get_socket(),&msgTosend,sizeof(MESSAGE),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
					if ( ret < 0){
						perror("error while sending the message \n");

						continue;
					}
					cout << "\nMessage being Sent :: ";
					cout << msgTosend.sContent;
				}



			}
		}

}

void *processThread(void *id){
	while(true){

		MESSAGE curMsg;

		if(!curNode->consoleQueue.empty() ){
#ifdef  DEBUG
			cout << "Process Thread : Holdback queue not empty \n";
#endif

		    curMsg = curNode->consoleQueue.pop();
		    curNode->sendQueue.push(curMsg);
		}
	}

}
void *holdbackThread(void *id){
	string printMsg;
	while (true){
		while(!curNode->holdbackQueue.empty()){
			MESSAGE curMsg;
			curMsg = curNode->holdbackQueue.pop();
			printMsg = string(curMsg.sContent);
			curNode->printQueue.push(printMsg);
		}
	}

}
void addUser(string ipaddress, int portNum){

	struct sockaddr_in client;
	client.sin_addr.s_addr = inet_addr(ipaddress.c_str());
	client.sin_family = AF_INET;
	client.sin_port = htons(portNum);
	curNode->listOfUsers.push_back(client);
}
int create_threads(pthread_t threads[NUM_THREADS]){
	int ret = 0 ;
	if(pthread_create(&threads[0],NULL,sendMsg,NULL)){
		ret++;
	}
	if(pthread_create(&threads[1],NULL,recvMsg,NULL)){
		ret++;
	}
	if(pthread_create(&threads[2],NULL,printConsole,NULL)){
		ret++;
	}
	if(pthread_create(&threads[3],NULL,processThread,NULL)){
		ret++;
	}
	if(pthread_create(&threads[4],NULL,holdbackThread,NULL)){
			ret++;
	}


	return ret;

}

int main(int argc, char *argv[]) {
	string ipaddress = findip();
	istringstream port(argv[2]);
	int portNum;
	int ret;
	pthread_t threads[NUM_THREADS];

	string username;
	string inpConsole;
	bool isSeq;

	if(argc < 3){
		isSeq = true;
		cout << "New Chat Started \n";
	}
	username = argv[1];
	port >> portNum;

	curServer = new udp_Server(ipaddress.c_str(),portNum);

	curNode = new chat_node(username,1,ipaddress,portNum);
	addUser(ipaddress,portNum);
	if(create_threads(threads)){
		perror("Error Creating threads \n");
		exit(1);
	}

	while(true){
		MESSAGE curMsg;
		string inpString;

		getline(cin,inpString);
		curMsg.sType = MESSAGE_TYPE_CHAT;
		strcpy(curMsg.sContent,inpString.c_str());
		curNode->consoleQueue.push(curMsg);


	}

	return 0;
}
