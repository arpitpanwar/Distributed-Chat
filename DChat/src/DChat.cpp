
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

string& trim_right_in_place(string& str);
void printAllUsers(map<string,string> clientMap ,bool isLeader);
using namespace std;

udp_Server* curServer;
chat_node* curNode;


string& trim_right_in_place(string& str) {
    size_t i = str.size();
    while(i > 0 && (isspace(str[i - 1])||(int)str[i-1]==0 )) { --i; };
    return str.erase(i, str.size());
}

void *printConsole(void *id){

	string msg;
	while(true){
		if(!curNode->printQueue.empty()){
#ifdef DEBUG
			cout << "Print thread entered \n";
#endif
			//msg = curNode->printQueue.front();
			msg =curNode->printQueue.pop();
			cout <<"Print on Console : " +msg + "\n";
		}
	}

}

void *recvMsg(void *id){

	struct sockaddr_in client;
	int bytes_recv;
	int addr_len = sizeof(struct sockaddr);
	string chatmsg;


	while(true){
		MESSAGE msg;
		vector<string> tokens;
		int seqNum = 0 ;
		chatmsg.resize(2048);
#ifdef DEBUG
		cout << "Receive Message thread entered \n";
#endif
		bytes_recv = recvfrom(curServer->get_socket(),&chatmsg[0],2048,0,
				(struct sockaddr *)&client,(socklen_t*)&addr_len);

		if(bytes_recv < 0){
			perror("Error while receiving message \n");
			continue;
		}
		istringstream iss(chatmsg);
		do{
			string token;
			iss >> token;
			trim_right_in_place(token);
			tokens.push_back(token);
		}while(iss);

		istringstream(tokens[1])>>seqNum;

		msg.sContent = tokens[0];
		msg.sType = MESSAGE_TYPE_CHAT;

		cout << "Message being received : " + msg.sContent + "\n";

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
			SENDMSG curSentmsg;

			if(!curNode->sendQueue.empty()){
				seqNum ++;
				curSentmsg = curNode->sendQueue.pop();
				curSentmsg.message += "\t";
				curSentmsg.message += to_string(seqNum);


				curSentmsg.message.resize(2048);

#ifdef DEBUG
			cout << "Send Message: Send Queue not empty\n";
			cout << curSentmsg.message;
#endif

				for (std::list<sockaddr_in>::const_iterator iterator = curSentmsg.memberIP.begin(), end = curSentmsg.memberIP.end(); iterator != end; ++iterator) {
					client = *iterator;

					ret = sendto(curServer->get_socket(),curSentmsg.message.c_str(),2048,0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
					if ( ret < 0){
						perror("error while sending the message \n");

						continue;
					}
					cout << "Message being sent : " +curSentmsg.message+"\n";

				}



			}
		}

}

void *processThread(void *id){
	while(true){

		MESSAGE curMsg;
		SENDMSG curMsgtosend;
		if(!curNode->consoleQueue.empty() ){
#ifdef  DEBUG
			cout << "Process Thread : Holdback queue not empty \n";
#endif
//			curMsg = curNode->holdbackQueue.front();
		    curMsg = curNode->consoleQueue.pop();
			curMsgtosend.message = curMsg.sContent;
			curMsgtosend.memberIP.assign(curNode->listOfUsers.begin(),curNode->listOfUsers.end());
			curNode->sendQueue.push(curMsgtosend);
		}
	}

}
void *holdbackThread(void *id){
	string printMsg;
	while (true){
		while(!curNode->holdbackQueue.empty()){
			MESSAGE curMsg;
			curMsg = curNode->holdbackQueue.pop();
			printMsg = curMsg.sContent;
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
		curMsg.sType = MESSAGE_TYPE_CHAT;
		getline(cin,curMsg.sContent);
		curNode->consoleQueue.push(curMsg);


	}

	return 0;
}
