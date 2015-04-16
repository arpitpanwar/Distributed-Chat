
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
#include <string>
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
udp_Server* heartBeatSendServer;
chat_node* curNode;

void populateLeader(LEADER *lead,char ip[],int portNum,char username[]);
void* heartbeatSend(void *);

int conductElection(chat_node* curNode, udp_Server* curServer, udp_Server* ackServer);

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

void addUserlist(char ipaddress[],int portNum, char username[],char rxsize[]){
	USERINFO temp;
	addUser(ipaddress,portNum,username);

	strcpy(temp.ipaddress,ipaddress);
	sprintf(temp.portnum,"%d",portNum);
	strcpy(temp.username,username);
	strcpy(temp.rxBytes,rxsize);

	curNode->listofUsers.push_back(temp);


}

void *printConsole(void *id){

	string msg;
	while(true){

#ifdef DEBUG
			cout << "Print thread entered \n";
#endif
			//msg = curNode->printQueue.front();
			msg =curNode->printQueue.pop();

			cout <<msg<<endl;

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

		if(msg.sType != MESSAGE_TYPE_ELECTION){

		client.sin_port = htons(ntohs(client.sin_port)+1);
		}
		else{
			client.sin_port = htons(ntohs(client.sin_port)-1);
		}
		{
			ret = sendto(ackServer->get_socket(),&ack,sizeof(ack),0,
					(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
			if ( ret < 0){
				perror("error while sending the message \n");
				continue;
			}
		}



		if((msg.sType == MESSAGE_TYPE_STATUS_JOIN)
				|| (msg.sType ==MESSAGE_TYPE_CHAT_NOSEQ) || (msg.sType == MESSAGE_TYPE_UPDATE)){

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
		else if ( msg.sType == MESSAGE_TYPE_LEADER){
			list<UserInfo>::iterator itr;

			string content = string(msg.sContent);
			curNode->mStatusmap.clear();
			curNode->mClientmap.erase(string(curNode->lead.sIpAddress)+":"+to_string(curNode->lead.sPort));
			curNode->listofSockets.clear();

			for(itr = curNode->listofUsers.begin(); itr != curNode->listofUsers.end(); ++itr){
				USERINFO user;

				user = *itr;
				if(strcmp(user.username,curNode->lead.sName)==0){
					curNode->listofUsers.erase(itr);
					break;
				}

			}
			char portNum[PORT_BUFSIZE];
			sscanf(portNum, "%d", &port);
			memcpy(curNode->lead.sIpAddress,msg.sContent,IP_BUFSIZE);
			memcpy(portNum,msg.sContent+IP_BUFSIZE,PORT_BUFSIZE);
			memcpy(curNode->lead.sName ,msg.sContent+IP_BUFSIZE+PORT_BUFSIZE,USERNAME_BUFSIZE);
			memcpy(curNode->rxBytes ,msg.sContent+IP_BUFSIZE+PORT_BUFSIZE+USERNAME_BUFSIZE,RXBYTE_BUFSIZE);

			curNode->lead.sPort = stoi(string(portNum));

			addSocket(curNode->lead.sIpAddress,curNode->lead.sPort);

			//strcpy(curNode->lead.sIpAddress,msg.sContent);

			//strcpy(msgTosend.sContent+IP_BUFSIZE,to_string(curNode->lead.sPort).c_str());
		//strcpy(msgTosend.sContent+IP_BUFSIZE+PORT_BUFSIZE,curNode->lead.sName);
			//	strcpy(msgTosend.sContent+IP_BUFSIZE+PORT_BUFSIZE
				//		+USERNAME_BUFSIZE,curNode->rxBytes);

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

int populatelistofUsers(char *users){
	USERINFO temp;
	int countNum = 0;
	for (list<USERINFO>::const_iterator iterator = curNode->listofUsers.begin(), end = curNode->listofUsers.end(); iterator != end; ++iterator) {
		temp = *iterator;
		memcpy(users,temp.ipaddress,IP_BUFSIZE);
		memcpy(users+IP_BUFSIZE,temp.portnum,PORT_BUFSIZE);
		memcpy(users+IP_BUFSIZE+PORT_BUFSIZE,temp.username,USERNAME_BUFSIZE);
		memcpy(users+IP_BUFSIZE+PORT_BUFSIZE+USERNAME_BUFSIZE,temp.rxBytes,RXBYTE_BUFSIZE);
		users+=IP_BUFSIZE + PORT_BUFSIZE+USERNAME_BUFSIZE+RXBYTE_BUFSIZE;
		countNum ++;
	}
	return countNum;
}


void sendlist(char *msg){
	int num,ret;
	char ip[IP_BUFSIZE];
	char portNum[PORT_BUFSIZE];
	char username[USERNAME_BUFSIZE];
	char rxsize[RXBYTE_BUFSIZE];
	int port;
	LISTMSG msgTosend;
	MESSAGE updateMsg;
	struct sockaddr_in client;
	struct timeval tv;
	char  ack[4];

	memcpy(ip,msg,IP_BUFSIZE);
	memcpy(portNum,msg+IP_BUFSIZE,PORT_BUFSIZE);
	memcpy(username,msg+IP_BUFSIZE+PORT_BUFSIZE,USERNAME_BUFSIZE);
	memcpy(rxsize,msg+IP_BUFSIZE+PORT_BUFSIZE+USERNAME_BUFSIZE,RXBYTE_BUFSIZE);

	sscanf(portNum, "%d", &port);

	addUserlist(ip,port,username,rxsize);
	client.sin_addr.s_addr = inet_addr(ip);
	client.sin_family = AF_INET;
	sscanf(portNum, "%d", &port);
	client.sin_port = htons(port);

	num = populatelistofUsers(msgTosend.listUsers);

	msgTosend.leaderPort = curNode->lead.sPort;
	strcpy(msgTosend.leaderip,curNode->lead.sIpAddress);
	msgTosend.numUsers = num;
	strcpy(msgTosend.leaderName,curNode->lead.sName);


	ret = sendto(curServer->get_socket(),&msgTosend,sizeof(LISTMSG),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
	if ( ret < 0){
		perror("error while sending the List Message \n");

	}

	updateMsg.sType = MESSAGE_TYPE_UPDATE;
	string cont = ip+string(":")+portNum+string(":")+username+string(":")+rxsize;
	strcpy(updateMsg.sContent,cont.c_str());

	for (std::list<sockaddr_in>::const_iterator iterator = curNode->listofSockets.begin(), end = curNode->listofSockets.end(); iterator != end; ++iterator) {
		client = *iterator;

		ret = sendto(curServer->get_socket(),&updateMsg,sizeof(MESSAGE),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
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


					ret = sendto(curServer->get_socket(),&updateMsg,sizeof(MESSAGE),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
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
	}


	addSocket(ip,port);
	MESSAGE update;
	string temp;
	update.sType = MESSAGE_TYPE_CHAT;
	strcpy(update.sContent, string(string("NOTICE:: ")+string(username)+string(" joined on:: ")+string(ip)+string(":")+to_string(port)).c_str());

	curNode->sendQueue.push(update);
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

		    	if(!curNode->bIsLeader && curMsg.sType == MESSAGE_TYPE_UPDATE){
		    			vector<string> tokens = split(string(curMsg.sContent),':');

		    			if(tokens.size()!=4){
		    				cout<<"Invalid update message received";
		    			}else{
		    				char ip[IP_BUFSIZE];
		    				char user[USERNAME_BUFSIZE];
		    				char rxsize[RXBYTE_BUFSIZE];
		    				strcpy(ip,tokens[0].c_str());
		    				strcpy(user,tokens[2].c_str());
		    				strcpy(rxsize,tokens[3].c_str());
		    				addUserlist(ip,atoi(tokens[1].c_str()),user,rxsize);

		    			}

		    	}else{
		    		if(curMsg.sType != MESSAGE_TYPE_UPDATE)
		    			curNode->sendQueue.push(curMsg);
		    	}
		    }

	}
	return (void*)NULL;
}

void *heartbeatThread(void *id){
	struct sockaddr_in client,recvClient;
	int ret;
	int addr_len = sizeof(struct sockaddr);
	struct timeval tv;
	char sendBeat[16],recvBeat[16];
	pthread_t sendThread;
	timeval start , end;

	tv.tv_sec = 2;
	gettimeofday(&start,NULL);
	while(true){

	//	if (setsockopt(heartBeatserver->get_socket(), SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	//		perror("Error while setting the timer for the heartbeat signal\n");
	//	}
		char recvBeat[16];
		int ret = recvfrom(heartBeatserver->get_socket(),&recvBeat,sizeof(recvBeat),0,(struct sockaddr *)&client,(socklen_t*)&addr_len);

		if(ret<0){
			gettimeofday(&end,NULL);
			if((start.tv_sec-end.tv_sec)/CLOCKS_PER_SEC>=10){

				// int rc = pthread_create(&sendThread, NULL, heartbeatSend, (void *)NULL);
				// if (rc){
				  //      perror("Error creating thread");
				 // }
				gettimeofday(&start,NULL);
			}
		}else{
				string received = recvBeat;

				if(received.compare(BEAT)==0){
					timeval curr;
					gettimeofday(&curr,NULL);
					string ip = string(inet_ntoa(client.sin_addr));
					int port = ntohs(client.sin_port);
					string key = ip+string(":")+to_string(port);

					curNode->mStatusmap[key] = curr.tv_sec;

				}
				else if(received.compare(to_string(MESSAGE_TYPE_ELECTION))){

					if(curNode->statusServer != ELECTION_HAPPENING){
						{
							int ret;
							char ackMsg[4] = "ACK";
							ret = sendto(ackServer->get_socket(),&ackMsg,sizeof(ackMsg),0,
													(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));							if ( ret < 0){
								perror("error while sending the message \n");
								continue;
							}
						}


						curNode->statusServer = ELECTION_HAPPENING;
						conductElection(curNode, curServer, ackServer);
					}
				}

		}

	}


}

void* heartbeatSend(void *id){
		struct sockaddr_in client;
		int ret;
		int addr_len = sizeof(struct sockaddr);
		struct timeval tv;
		char sendBeat[16],recvBeat[16];
		while(true){
				for (std::list<sockaddr_in>::const_iterator iterator = curNode->listofSockets.begin(), end = curNode->listofSockets.end(); iterator != end; ++iterator) {
					client = *iterator;
					strcpy(sendBeat,BEAT);
					sockaddr_in client;
					client.sin_addr.s_addr = iterator->sin_addr.s_addr;
					client.sin_family = iterator->sin_family;
					client.sin_port = htons(ntohs(iterator->sin_port)+2);

					ret = sendto(heartBeatserver->get_socket(),&sendBeat,sizeof(sendBeat),0,(struct sockaddr *)&client,(socklen_t)addr_len);
					if ( ret < 0){
						perror("error while sending the heartbeat signal \n");
					}

				}

		map<string,double>::iterator it = curNode->mStatusmap.begin();
		timeval start;
		gettimeofday(&start,NULL);
		//double curTime = clock()/CLOCKS_PER_SEC;
		while(it != curNode->mStatusmap.end()){

				if((  start.tv_sec - it->second) >=20){
					if(!curNode->bIsLeader){
						if(curNode->statusServer != ELECTION_HAPPENING){


							curNode->statusServer = ELECTION_HAPPENING;
							conductElection(curNode,heartBeatserver,ackServer);
						}

					}
					//TODO To decide who should remove the users
				}
				it++;
		}

		sleep(5);
		}

}

void *holdbackThread(void *id){
	string printMsg;
	while (true){

			MESSAGE curMsg;
			curMsg = curNode->holdbackQueue.pop();


			printMsg = string(curMsg.sContent);
			curNode->printQueue.push(printMsg);

	}

}



void populatesocketClient(char userList[],int numUser){
	int i;
	char ipaddress[IP_BUFSIZE];
	char port[PORT_BUFSIZE];
	char username[USERNAME_BUFSIZE];
	char rxsize[RXBYTE_BUFSIZE];
	int  portNum;
	char *ptr = userList;
	for (int i = 0 ; i < numUser ; i++){

		memcpy(ipaddress,ptr,IP_BUFSIZE);
		memcpy(port,ptr+IP_BUFSIZE,PORT_BUFSIZE);
		memcpy(username,ptr+IP_BUFSIZE+PORT_BUFSIZE,USERNAME_BUFSIZE);
		memcpy(rxsize,ptr+IP_BUFSIZE+PORT_BUFSIZE+USERNAME_BUFSIZE,RXBYTE_BUFSIZE);
		istringstream temp(port);
		temp >> portNum;
		addUserlist(ipaddress,portNum,username,rxsize);
		ptr +=IP_BUFSIZE + PORT_BUFSIZE+USERNAME_BUFSIZE+RXBYTE_BUFSIZE;
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
	if(pthread_create(&threads[5],NULL,heartbeatThread,NULL)){
		ret++;
	}
	if(pthread_create(&threads[6],NULL,heartbeatSend,NULL)){
			ret++;
	}

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
	char rxsize[RXBYTE_BUFSIZE];
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
	string bytes = getRxBytes();
	strcpy(rxsize,bytes.c_str());

	curServer = new udp_Server(ipaddress,portNum);
	ackServer = new udp_Server(ipaddress,portNum+1);
	heartBeatserver = new udp_Server(ipaddress,portNum+2);
	curNode = new chat_node(username,entry,ipaddress,portNum);

	if(isSeq){
		strcpy(curNode->rxBytes,rxsize);
		curNode->statusServer = NORMAL_OPERATION;
		addUserlist(ipaddress,portNum,username,rxsize);
		addSocket(ipaddress,portNum);
		populateLeader(&curNode->lead,ipaddress,portNum,username);
		curNode->bIsLeader =true;
	}
	else{
		strcpy(curNode->rxBytes,rxsize);
		curNode->statusServer = NORMAL_OPERATION;
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


		strcpy(toSendip,tokens[0].c_str());
		sendPort = stoi(tokens[1]);

		seqClient.sin_addr.s_addr = inet_addr(toSendip);
		seqClient.sin_family = AF_INET;
		seqClient.sin_port = htons(sendPort);

		//Entering details of the user to be sent so that it can join the group.
		joinMsg.sType = MESSAGE_TYPE_STATUS_JOIN;

		memcpy(joinMsg.sContent,ipaddress,IP_BUFSIZE);
		sprintf(joinMsg.sContent+IP_BUFSIZE,"%d",portNum);
		memcpy(joinMsg.sContent+IP_BUFSIZE+PORT_BUFSIZE,username,USERNAME_BUFSIZE);
		memcpy(joinMsg.sContent+IP_BUFSIZE+PORT_BUFSIZE+USERNAME_BUFSIZE,bytes.c_str(),RXBYTE_BUFSIZE);
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
		char ack[4];
		seqClient.sin_port = htons(ntohs(seqClient.sin_port )+1);
		ret = sendto(ackServer->get_socket(),&ack,sizeof(ack),0,
						(struct sockaddr *)&seqClient,(socklen_t)sizeof(struct sockaddr));
		if ( ret < 0){
			perror("error while sending the message \n");
		}

		populatesocketClient(userListMsg.listUsers,userListMsg.numUsers);
		addSocket(userListMsg.leaderip,userListMsg.leaderPort);
		strcpy(curNode->lead.sIpAddress,userListMsg.leaderip);
		curNode->lead.sPort = userListMsg.leaderPort;
		strcpy(curNode->lead.sName ,userListMsg.leaderName);
	}


	if(create_threads(threads)){
		perror("Error Creating threads \n");
		exit(1);
	}


	while(true){
		MESSAGE curMsg;
		string inpString;
		getline(cin,inpString);

		if(inpString.size() >= MESSAGE_SIZE){
			cout<<INVALID_MESSAGE_SIZE<<endl;
			continue;
		}

		curMsg.sType = MESSAGE_TYPE_CHAT;
		strcpy(curMsg.sContent,inpString.c_str());


		if(curNode->bIsLeader){
			string content = curMsg.sContent;
			string key = curNode->lead.sIpAddress+string(":")+to_string(curNode->lead.sPort);

			if(curNode->mClientmap.find(key) != curNode->mClientmap.end()){
				content = curNode->mClientmap[key]+string(":: ")+content;
				strcpy(&curMsg.sContent[0],content.c_str());
			}
		}
		curNode->consoleQueue.push(curMsg);
	}

	return 0;
}
