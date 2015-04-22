
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
	//cout << "\nAdding socket list "<<portNum<<endl;
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
	long lastSeqNum = 0;
	while(true){
		struct sockaddr_in client;
		MESSAGE msg;
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

		if((msg.sType != MESSAGE_TYPE_ELECTION) && (msg.sType != MESSAGE_TYPE_LEADER)){

		client.sin_port = htons(ntohs(client.sin_port)+1);
		}
		else{
			client.sin_port = htons(ntohs(client.sin_port)-1);
		}

			ret = sendto(ackServer->get_socket(),&ack,sizeof(ack),0,
					(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
			if ( ret < 0){
				perror("error while sending the message \n");
				continue;
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
			list<USERINFO> userList = curNode->getUserList();
			for(itr = userList.begin(); itr != userList.end(); ++itr){
				USERINFO user;

				user = *itr;
				if(strcmp(user.username,curNode->lead.sName)==0){
					curNode->listofUsers.erase(itr);
					break;
				}

			}
			//delete &userList;

			char portNum[PORT_BUFSIZE];
			sscanf(portNum, "%d", &port);
			memcpy(curNode->lead.sIpAddress,msg.sContent,IP_BUFSIZE);
			memcpy(portNum,msg.sContent+IP_BUFSIZE,PORT_BUFSIZE);
			memcpy(curNode->lead.sName ,msg.sContent+IP_BUFSIZE+PORT_BUFSIZE,USERNAME_BUFSIZE);
			memcpy(curNode->rxBytes ,msg.sContent+IP_BUFSIZE+PORT_BUFSIZE+USERNAME_BUFSIZE,RXBYTE_BUFSIZE);

			curNode->lead.sPort = stoi(string(portNum));

			addSocket(curNode->lead.sIpAddress,curNode->lead.sPort);

			curNode->statusServer = NORMAL_OPERATION;


			lastSeqNum = 0;
			curNode->lastSeqNum = 0 ;

		}else{

			if(msg.sType == MESSAGE_TYPE_REMOVE_USER){

				if(!curNode->bIsLeader){
					string content = msg.sContent;
					cout<<"Content is "<<content<<endl;
					vector<string> tokens = split(content,':');

					list<USERINFO> userList = curNode->getUserList();
					list<USERINFO>::iterator itr  = userList.begin();

					while(itr!=userList.end()){
						USERINFO user = *itr;

						if((strcmp(user.ipaddress,tokens[0].c_str())==0) && (strcmp(user.portnum,tokens[1].c_str())==0)){
							curNode->listofUsers.remove(user);

							break;
						}
						++itr;
					}
					map<string,string>::iterator it;

					it =curNode->mClientmap.find(content);

					if(it != curNode->mClientmap.end()){
						curNode->mClientmap.erase(it);
					}

				}


			}else{
				//if(lastSeqNum!= msg.lSequenceNums)
				{
					if(lastSeqNum == 0 )
						curNode->lastSeqNum = msg.lSequenceNums -1;

					lastSeqNum = msg.lSequenceNums;
					curNode->holdbackQueue.push(msg);
				}
			}
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
	long seqNum = 0;
	struct timeval tv;
	int flag = 0;
	int firstTime = 0;
	while(true){

		MESSAGE msgTosend;
		char  ack[4];

		msgTosend = curNode->sendQueue.front();

		if(curNode->bIsLeader){
			if((flag == 0) ||(firstTime ==1))
				seqNum++;
			msgTosend.sType = MESSAGE_TYPE_CHAT;
			string content = msgTosend.sContent;
			list<USERINFO> users = curNode->listofUsers;
			msgTosend.lSequenceNums = seqNum;
			firstTime = 1;

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
		flag = 0;


#ifdef DEBUG
		cout << "Send Message: Send Queue not empty\n";
		cout << curSentmsg.message;
#endif
		list<sockaddr_in> sockets = curNode->getSocketList();
		for (std::list<sockaddr_in>::const_iterator iterator = sockets.begin(), end = sockets.end(); iterator != end; ++iterator) {
			client = *iterator;

			ret = sendto(curServer->get_socket(),&msgTosend,sizeof(MESSAGE),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
			if ( ret < 0){
				perror("error while sending the message \n");
				continue;
			}
			{
//				cout<<"Message sent to with ret value :"<<ret<<endl;
//				cout<<"client port number : "<<ntohs(client.sin_port)<<endl;
			}

			tv.tv_sec = 2;
			tv.tv_usec = 0;
			if (setsockopt(ackServer->get_socket(), SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
				perror("Error while setting a time constraint on the socket");
			}
			{
				int timeout = 0;
				struct sockaddr_in ackClient;

				while(timeout < 2){
					char ackMsg[4];
					int ret;
					ret = recvfrom(ackServer->get_socket(),ackMsg,sizeof(ackMsg), 0, (struct sockaddr *) &ackClient, (socklen_t *)&addr_len);

					if(ret<0){
						//	perror("Message being resent  \n");
						ret = sendto(curServer->get_socket(),&msgTosend,sizeof(MESSAGE),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
						timeout++;
						if(timeout==2){
							flag = 1;
						}
					}
					else{
						{
						//	cout<<"Message received to with ret value :"<<ret<<endl;
							//cout<<"Ack client port number : "<<ntohs(ackClient.sin_port)<<endl;
						}
						if(strcmp(ackMsg,"ACK")==0)
							break;
						else{
							timeout++;
							if(timeout==2){
								flag = 1;
							}
						}

					}
				}
			}
#ifdef PRINT
			cout << "\nMessage being Sent :: ";
			cout << msgTosend.sContent;
#endif
		}

		if(flag == 0 ){
//			cout<<"\nMessage being popped"<<msgTosend.sContent<<endl;
			curNode->sendQueue.pop();
		}
	}

}

int populatelistofUsers(char *users){
	USERINFO temp;
	int countNum = 0;
	list<USERINFO> userList = curNode->getUserList();
	for (list<USERINFO>::const_iterator iterator = userList.begin(), end = userList.end(); iterator != end; ++iterator) {
		temp = *iterator;
		memcpy(users,temp.ipaddress,IP_BUFSIZE);
		memcpy(users+IP_BUFSIZE,temp.portnum,PORT_BUFSIZE);
		memcpy(users+IP_BUFSIZE+PORT_BUFSIZE,temp.username,USERNAME_BUFSIZE);
		memcpy(users+IP_BUFSIZE+PORT_BUFSIZE+USERNAME_BUFSIZE,temp.rxBytes,RXBYTE_BUFSIZE);
		users+=IP_BUFSIZE + PORT_BUFSIZE+USERNAME_BUFSIZE+RXBYTE_BUFSIZE;
		countNum ++;
	}
	//delete &userList;
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
	list<sockaddr_in> sockets = curNode->getSocketList();
	for (std::list<sockaddr_in>::const_iterator iterator = sockets.begin(), end = sockets.end(); iterator != end; ++iterator) {
		client = *iterator;

		ret = sendto(curServer->get_socket(),&updateMsg,sizeof(MESSAGE),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
		if ( ret < 0){
			perror("error while sending the message \n");
			continue;
		}

		tv.tv_sec = 2;
		tv.tv_usec = 0;
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
	//delete &sockets;


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
	tv.tv_usec = 0;
	gettimeofday(&start,NULL);
	while(true){


		char recvBeat[16];
		int ret = recvfrom(heartBeatserver->get_socket(),&recvBeat,sizeof(recvBeat),0,(struct sockaddr *)&client,(socklen_t*)&addr_len);

		if(ret<0){
			gettimeofday(&end,NULL);
			if((start.tv_sec-end.tv_sec)/CLOCKS_PER_SEC>=2){
				gettimeofday(&start,NULL);
			}
		}else{
				string received = recvBeat;
				if(received.compare(BEAT)==0){
					timeval curr;
					gettimeofday(&curr,NULL);
					string ip = string(inet_ntoa(client.sin_addr));
					int port = ntohs(client.sin_port);
					string key = ip+string(":")+to_string(port-2);

					curNode->mStatusmap[key] = curr.tv_sec;

				}

				else if(received.compare(to_string(MESSAGE_TYPE_ELECTION))== 0){

					if(curNode->statusServer != ELECTION_HAPPENING){
						{
							int ret;
							char ackMsg[4] = "ACK";
							client.sin_port = htons(ntohs(client.sin_port)-1);

							cout<<"Detected election from other client"<<endl;
							ret = sendto(ackServer->get_socket(),&ackMsg,sizeof(ackMsg),0,
									(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
						}


						curNode->statusServer = ELECTION_HAPPENING;
						cout<<"Entering election via heartbeatThread\n";
						conductElection(curNode,heartBeatserver,ackServer);
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
				list<sockaddr_in> sockets = curNode->getSocketList();
				for (std::list<sockaddr_in>::const_iterator iterator = sockets.begin(), end = sockets.end(); iterator != end; ++iterator) {
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
		//delete &sockets;
		map<string,double>::iterator it = curNode->mStatusmap.begin();
		timeval start;
		gettimeofday(&start,NULL);
		//double curTime = clock()/CLOCKS_PER_SEC;
		while(it != curNode->mStatusmap.end()){
				if((  start.tv_sec - it->second) >=10){
					if(!curNode->bIsLeader){
						if(curNode->statusServer != ELECTION_HAPPENING){
							curNode->statusServer = ELECTION_HAPPENING;
							conductElection(curNode,heartBeatserver,ackServer);
						}

					}else{
						list<USERINFO> userList = curNode->getUserList();
						list<USERINFO>::iterator itr = userList.begin();
						MESSAGE removeMsg,removeChat;
						string ipport = it->first;
						map<string,string>::iterator mapIt = curNode->mClientmap.find(ipport);

						if(mapIt!=curNode->mClientmap.end()){
							curNode->mClientmap.erase(mapIt);
						}

						list<sockaddr_in> sockets = curNode->getSocketList();
						list<sockaddr_in>::iterator socketItr = sockets.begin();
						vector<string> tokens = split(ipport,':');

						while(socketItr!=sockets.end()){

							sockaddr_in sock = *socketItr;

							string ip = string(inet_ntoa(sock.sin_addr));
							string port = to_string(ntohs(sock.sin_port));

							if((strcmp(ip.c_str(),tokens[0].c_str())==0) & (strcmp(port.c_str(),tokens[1].c_str())==0)){
									sockets.erase(socketItr);
									break;
							}
							++socketItr;
						}
						curNode->listofSockets = sockets;

						while(itr!=userList.end()){
							USERINFO user = *itr;
							if((strcmp(user.ipaddress,tokens[0].c_str())==0) && (strcmp(user.portnum,tokens[1].c_str())==0)){
								curNode->listofUsers.remove(user);

								removeMsg.sType = MESSAGE_TYPE_REMOVE_USER;
								strcpy(removeMsg.sContent,it->first.c_str());
								curNode->consoleQueue.push(removeMsg);

								string remove = "Notice "+string(user.username)+" left the group or crashed";
								removeChat.sType=MESSAGE_TYPE_CHAT;
								strcpy(removeChat.sContent,remove.c_str());

								curNode->consoleQueue.push(removeChat);

								break;
							}

							++itr;
						}



						map<string,double>::iterator statIt = curNode->mStatusmap.find(ipport);

						if(statIt!=curNode->mStatusmap.end()){
							curNode->mStatusmap.erase(statIt);
							break;
						}
					}
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
		curMsg = curNode->holdbackQueue.front();

	//	if(curMsg.lSequenceNums == (curNode->lastSeqNum + 1))
		{
			curNode->holdbackQueue.pop();
			curNode->lastSeqNum = curMsg.lSequenceNums;
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
	curNode = new chat_node(username,entry,ipaddress,portNum,0);

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
