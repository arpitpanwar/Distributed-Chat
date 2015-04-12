
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
#include<string>
#include<sstream>
#include<pthread.h>
#include<vector>
#include<cctype>
#include<algorithm>
#include "headers/udpserver.h"
#include "headers/defs.h"
#include "headers/chatstructures.h"

using namespace std;

udp_Server* curServer;
udp_Server* heartBeatserver;
udp_Server* ackServer;
chat_node* curNode;

void populateLeader(LEADER *lead,char ip[],int portNum,char username[]);



void addUser(char ipaddress[],int portNum,char username[]){

	string ipPort = ipaddress+string(":")+to_string(portNum);

	string user = username;

	if(curNode->mClientmap.find(ipPort) == curNode->mClientmap.end()){

	curNode->mClientmap[ipPort] = user;
	}

}

void addSocket(char ipaddress[], int portNum){

	struct sockaddr_in client;
	client.sin_addr.s_addr = inet_addr(ipaddress);
	client.sin_family = AF_INET;
	client.sin_port = htons(portNum);
	curNode->listofSockets.push_back(client);
}

void addUserlist(char ipaddress[],int portNum, char username[]){
	USERINFO temp;
	addUser(ipaddress,portNum,username);

	strcpy(temp.ipaddress,ipaddress);
	sprintf(temp.portnum,"%d",portNum);
	strcpy(temp.username,username);

	curNode->listofUsers.push_back(temp);


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
			cout <<msg<<endl;
		}
	}

}

void *recvMsg(void *id){

	int addr_len = sizeof(struct sockaddr);
	int ret;
	while(true){
		struct sockaddr_in client;
		MESSAGE msg;
		long timestamp;
		char ack[4] ="ACK";

#ifdef DEBUG
		cout << "Receive Message thread entered \n";
#endif

		ret = recvfrom(curServer->get_socket(),&msg,sizeof(MESSAGE),0,
				(struct sockaddr *)&client,(socklen_t*)&addr_len);
		if (ret < 0){
			perror("error while sending the message \n");
			//continue;
		}

		string ip = string(inet_ntoa(client.sin_addr));
		int port = ntohs(client.sin_port);

		client.sin_port = htons(ntohs(client.sin_port)+1);

		if(msg.sType != MESSAGE_TYPE_STATUS_JOIN){
				ret = sendto(ackServer->get_socket(),&ack,sizeof(ack),0,
						(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
				if ( ret < 0){
					perror("error while sending the message \n");
					continue;
				}
			}



		if((msg.sType == MESSAGE_TYPE_STATUS_JOIN)
				|| (msg.sType ==MESSAGE_TYPE_CHAT_NOSEQ)	){

			if(msg.sType ==MESSAGE_TYPE_CHAT_NOSEQ){
				string content = msg.sContent;
				string key = ip+string(":")+to_string(port);
				if(curNode->mClientmap.find(key) != curNode->mClientmap.end()){
					content = curNode->mClientmap[key]+string(":: ")+content;
					strcpy(&msg.sContent[0],content.c_str());
				}
			}

			curNode->consoleQueue.push(msg);

		}
		else{

			curNode->holdbackQueue.push(msg);
		}

#ifdef PRINT
		cout << "\nMessage being received :: " ;
		cout << msg.sContent;
		std::cout.flush();
#endif




	}
}
void *sendMsg(void *id){
	struct sockaddr_in client;
	int ret;
	int addr_len = sizeof(struct sockaddr);
	int seqNum = 0;
	struct timeval tv;

	while(true){

			MESSAGE msgTosend;
			char  ack[4];
			if(!curNode->sendQueue.empty()){

				msgTosend = curNode->sendQueue.pop();
//				strcpy(msgTosend.ackIp,curNode->ipAddress);
//				msgTosend.ackPort = ackServer->get_portNum();
				if(curNode->bIsLeader){

					seqNum++;
					msgTosend.sType = MESSAGE_TYPE_CHAT;
					string content = msgTosend.sContent;

					list<USERINFO> users = curNode->listofUsers;



					msgTosend.lSequenceNums = seqNum;
				}
				else{
					if(msgTosend.sType == MESSAGE_TYPE_STATUS_JOIN){
						// This is to indicate the join message is coming from some other user and
						///this message is forwarded to the sequencer.
						msgTosend.sType = MESSAGE_TYPE_STATUS_JOIN;
					}else{
						msgTosend.sType = MESSAGE_TYPE_CHAT_NOSEQ;
					}

				}


#ifdef DEBUG
			cout << "Send Message: Send Queue not empty\n";
			cout << curSentmsg.message;
#endif

				for (std::list<sockaddr_in>::const_iterator iterator = curNode->listofSockets.begin(), end = curNode->listofSockets.end(); iterator != end; ++iterator) {
					client = *iterator;

					ret = sendto(curServer->get_socket(),&msgTosend,sizeof(MESSAGE),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
					if ( ret < 0){
						perror("error while sending the message \n");
						continue;
					}

					tv.tv_sec = 2;
					if (setsockopt(ackServer->get_socket(), SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
					    perror("Error while setting a time constraint on the socket");
					}
					{
						int timeout = 0;
						struct sockaddr_in ackClient;
						char ackMsg[4];
						while(timeout < 2){
							if(ackServer->get_message(ackClient,ackMsg,sizeof(ack))<0){
								perror("Message being resent \n");


								ret = sendto(curServer->get_socket(),&msgTosend,sizeof(MESSAGE),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
								if ( ret < 0){

									//Declare that particular client as dead
									perror("error while sending the message \n");
									continue;
								}

								timeout++;
							}
							else{

								if(strcmp(ackMsg,"ACK")==0)
									break;
							}
						}
					}
#ifdef PRINT
					cout << "\nMessage being Sent :: ";
					cout << msgTosend.sContent;
#endif
				}
			}
		}

}

int populatelistofUsers(char *users){
	USERINFO temp;
	int countNum = 0;
	for (list<USERINFO>::const_iterator iterator = curNode->listofUsers.begin(), end = curNode->listofUsers.end(); iterator != end; ++iterator) {
		temp = *iterator;
		memcpy(users,temp.ipaddress,IP_BUFSIZE);
		memcpy(users+IP_BUFSIZE,temp.portnum,PORT_BUFSIZE);
		users+=IP_BUFSIZE + PORT_BUFSIZE;
		countNum ++;
	}
	return countNum;
}


void sendlist(char *msg){
	int num,ret;
	char ip[IP_BUFSIZE];
	char portNum[PORT_BUFSIZE];
	char username[USERNAME_BUFSIZE];
	int port;
	LISTMSG msgTosend;
	struct sockaddr_in client;

	memcpy(ip,msg,IP_BUFSIZE);
	memcpy(portNum,msg+IP_BUFSIZE,PORT_BUFSIZE);
	memcpy(username,msg+IP_BUFSIZE+PORT_BUFSIZE,USERNAME_BUFSIZE);
	sscanf(portNum, "%d", &port);

	addUserlist(ip,port,username);
	client.sin_addr.s_addr = inet_addr(ip);
	client.sin_family = AF_INET;
	sscanf(portNum, "%d", &port);
	client.sin_port = htons(port);

	num = populatelistofUsers(msgTosend.listUsers);

	msgTosend.leaderPort = curNode->lead.sPort;
	strcpy(msgTosend.leaderip,curNode->lead.sIpAddress);
	msgTosend.numUsers = num;

	ret = sendto(curServer->get_socket(),&msgTosend,sizeof(LISTMSG),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
	if ( ret < 0){
		perror("error while sending the List Message \n");

	}
	addSocket(ip,port);


}
void *processThread(void *id){

	while(true){

		MESSAGE curMsg;

#ifdef  DEBUG
			cout << "Process Thread : Holdback queue not empty \n";
#endif

		    curMsg = curNode->consoleQueue.pop();

		    if(curNode->bIsLeader && curMsg.sType == MESSAGE_TYPE_STATUS_JOIN){

		    	sendlist(curMsg.sContent);

		    }else{
		    	curNode->sendQueue.push(curMsg);
		    }

	}

}
//void *heartbeatThread(void *id){
//	struct sockaddr_in client,recvClient;
//	int ret;
//	int addr_len = sizeof(struct sockaddr);
//	struct timeval tv;
//	while(true){
//		char sendBeat[16],recvBeat[16];
//		sleep(10000); //Sleep for 10 seconds
//		if(curNode->bIsLeader){
//			for (std::list<sockaddr_in>::const_iterator iterator = curNode->listofSockets.begin(), end = curNode->listofSockets.end(); iterator != end; ++iterator) {
//				client = *iterator;
//				strcpy(sendBeat,"BEAT");
//				ret = sendto(ackServer->get_socket(),&sendBeat,sizeof(sendBeat),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
//				if ( ret < 0){
//					perror("error while sending the heartbeat signal \n");
//				}
//				tv.tv_sec = 5;
//				if (setsockopt(ackServer->get_socket(), SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
//					perror("Error while setting the timer for the heartbeat signal\n");
//				}
//				//TODO: NEED to check the logic for detecting dead clients and continue the heartbeat signal
//				if((ackServer->get_message(recvClient,recvBeat,sizeof(recvBeat)) < 0)){
//					perror("Timed out in receving the heartbeat \n ");
//					cout << "Declare the particular client dead\n";
//				}
//				if(strcmp(sendBeat,recvBeat)!=0){
//					cout << "heartbeat received is not right Need to conduct elections \n";
//				}
//			}
//		}
//		else{
//			tv.tv_sec = 5;
//			if (setsockopt(ackServer->get_socket(), SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
//					perror("Error while setting the timer for the heartbeat signal\n");
//			}
//			ret = recvfrom(ackServer->get_socket(),&recvBeat,sizeof(recvBeat),0,(struct sockaddr *)&client,(socklen_t*)&addr_len);
//			if(ret < 0){
//				perror("Timed out in receiving the heartbeat from the sequencer");
//				cout << "conduct elections ";
//			}
////TODO: Need to check the string that is being received ;
//			strcpy(sendBeat,"BEAT");
//			ret = sendto(ackServer->get_socket(),&sendBeat,sizeof(sendBeat),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
//			if ( ret < 0){
//				perror("error while sending the heartbeat signal \n");
//			}
//		}
//
//	}
//
//}
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



void populatesocketClient(char userList[],int numUser){
	int i;
	char ipaddress[IP_BUFSIZE];
	char port[PORT_BUFSIZE];
	char username[USERNAME_BUFSIZE];
	int  portNum;
	char *ptr = userList;
	for (int i = 0 ; i < numUser ; i++){

		memcpy(ipaddress,ptr,IP_BUFSIZE);
		memcpy(port,ptr+IP_BUFSIZE,PORT_BUFSIZE);
		memcpy(username,ptr+IP_BUFSIZE+PORT_BUFSIZE,USERNAME_BUFSIZE);
		istringstream temp(port);
		temp >> portNum;
		addUserlist(ipaddress,portNum,username);
		ptr +=IP_BUFSIZE + PORT_BUFSIZE+USERNAME_BUFSIZE;
	}

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
//	if(pthread_create(&threads[5],NULL,heartbeatThread,NULL)){
//		ret++;
//	}

	return ret;

}

int main(int argc, char *argv[]) {

	if((argc==1) | (argv == NULL)){
		cout<<"Invalid number of arguments"<<endl;
		cout<<USAGE<<endl;
		return 1;
	}

	char ipaddress[IP_BUFSIZE];
	char username[USERNAME_BUFSIZE];
	int portNum;
	int entry;
	bool isSeq;
	pthread_t threads[NUM_THREADS];

	if(argc == 2 ){
		isSeq = true;
		entry = 1;
		cout << "New Chat Started \n";
	}
	if(argc == 3){
		isSeq = false;
		entry = 0;
		cout << "Joining a existing chat \n";
	}

	portNum = getOpenPort();
	cout<<"Port is:"<<portNum<<endl;
	strcpy(username,argv[1]);
	strcpy(ipaddress,findip().c_str());
	curServer = new udp_Server(ipaddress,portNum);
	ackServer = new udp_Server(ipaddress,portNum+1);
	heartBeatserver = new udp_Server(ipaddress,portNum+2);
	curNode = new chat_node(username,entry,ipaddress,portNum);

	if(isSeq){
		addUserlist(ipaddress,portNum,username);
		addSocket(ipaddress,portNum);
		populateLeader(&curNode->lead,ipaddress,portNum,username);
		curNode->bIsLeader =true;
	}
	else{
		curNode->bIsLeader = false;
		MESSAGE joinMsg;
		LISTMSG userListMsg;
		struct sockaddr_in seqClient;
		char toSendip[IP_BUFSIZE];
		int sendPort;
		int ret;
		int addr_len = sizeof(struct sockaddr);
		string ipPort = argv[2];
		vector<string> tokens = split(ipPort,':');

		if(tokens.size()!=2){
			cout<<"Invalid arguments"<<endl;
			cout<<USAGE<<endl;
			return 1;
		}

		//port >> sendPort;
		strcpy(toSendip,tokens[0].c_str());
		sendPort = stoi(tokens[1]);

		seqClient.sin_addr.s_addr = inet_addr(toSendip);
		seqClient.sin_family = AF_INET;
		seqClient.sin_port = htons(sendPort);

		//Entering details of the user to be sent so that it can join the group.
		joinMsg.sType = MESSAGE_TYPE_STATUS_JOIN;
//		strcpy(joinMsg.ackIp,curNode->ipAddress);
//		joinMsg.ackPort = ackServer->get_portNum();
		memcpy(joinMsg.sContent,ipaddress,IP_BUFSIZE);
		sprintf(joinMsg.sContent+IP_BUFSIZE,"%d",portNum);
		memcpy(joinMsg.sContent+IP_BUFSIZE+PORT_BUFSIZE,username,USERNAME_BUFSIZE);
		ret = sendto(curServer->get_socket(),&joinMsg,sizeof(MESSAGE),0,(struct sockaddr *)&seqClient,(socklen_t)sizeof(struct sockaddr));
		if ( ret < 0){
			perror("error while sending the message \n");
			//continue;
		}
		ret = recvfrom(curServer->get_socket(),&userListMsg,sizeof(LISTMSG),0,
				(struct sockaddr *)&seqClient,(socklen_t*)&addr_len);
		if ( ret < 0){
			perror("error while sending the message \n");
			//continue;
		}
		populatesocketClient(userListMsg.listUsers,userListMsg.numUsers);
		addSocket(userListMsg.leaderip,userListMsg.leaderPort);

	}


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

		string content = curMsg.sContent;
		string key = curNode->lead.sIpAddress+string(":")+to_string(curNode->lead.sPort);

		if(curNode->mClientmap.find(key) != curNode->mClientmap.end()){
			content = curNode->mClientmap[key]+string(":: ")+content;
			strcpy(&curMsg.sContent[0],content.c_str());
		}

		curNode->consoleQueue.push(curMsg);
	}

	return 0;
}
