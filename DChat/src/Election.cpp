/*
 * Election.cpp
 *
 *  Created on: Apr 11, 2015
 *      Author: Jitesh Gupta
 */
#include "headers/udpserver.h"
#include "headers/defs.h"
#include "headers/chatstructures.h"
#include <string.h>
#include <sstream>
#include <arpa/inet.h>
void sendLeaderMessage(chat_node* curNode, udp_Server* curServer, udp_Server* ackServer, USERINFO user);

vector<string> split(string s, char delim);
int sendElectionMessage(chat_node* curNode, udp_Server* curServer, udp_Server* ackServer, USERINFO user, int numMsgReceived);

int conductElection(chat_node* curNode, udp_Server* curServer, udp_Server* ackServer){
	cout << "Conducting Elections...\n";
	int ret = 0 ;
	int val;
	//Check if your node has the highest port number.
	list<UserInfo>::iterator itr,itr2;
	bool isHighest = true;
	int numMsgReceived = 0;
	list<USERINFO> userList = curNode->getUserList();
#ifdef DEBUG
	cout << curNode->listofUsers.size();
	cout << curNode->listofSockets.size();
#endif
	for(itr = curNode->listofUsers.begin(); itr != curNode->listofUsers.end(); ++itr){
		struct UserInfo user = *itr;

		if((strcmp(user.username,curNode->lead.sName)==0) & (strcmp(user.ipaddress,curNode->lead.sIpAddress)==0) & ((atoi(user.portnum) == curNode->lead.sPort)) ){

			curNode->listofUsers.erase(itr);
			//userList.erase(itr);
			break;
		}
	}

	userList = curNode->getUserList();

	for(itr = userList.begin(); itr != userList.end(); ++itr){
		struct UserInfo user = *itr;

		if((atoi(user.portnum) != curNode->portNum)	| (strcmp((user.ipaddress),curNode->ipAddress)!= 0)){

			if(strcmp(curNode->rxBytes,user.rxBytes) < 0){
				isHighest = false;
				numMsgReceived =  sendElectionMessage(curNode, curServer, ackServer, user, numMsgReceived);


			}else{
				if(strcmp(curNode->rxBytes,user.rxBytes) == 0){
					if(curNode->portNum < atoi(user.portnum)){
						isHighest = false;
						numMsgReceived = sendElectionMessage(curNode, curServer, ackServer, user, numMsgReceived);
					}
				}
			}

		}
	}

	//delete &userList;

	cout << "isHighest:"<<isHighest<<endl;
	cout <<" numMsg Recevied:"<<numMsgReceived <<endl;
	//TODO Declare Yourself as the leader
	//Checking if the size of listofUsers = 2
	if((curNode->listofUsers.size() == 1) || (isHighest == true) || (numMsgReceived == 0)){
		cout << "I am the leader\n";
		curNode->bIsLeader = true;
		curNode->mStatusmap.clear();
		curNode->lastSeqNum = 0;
		curNode->mClientmap.erase(string(curNode->lead.sIpAddress)+":"+to_string(curNode->lead.sPort));
		curNode->listofSockets.clear();
		list<USERINFO> userList = curNode->getUserList();
		for(itr = userList.begin(); itr != userList.end(); ++itr){
			USERINFO user;

			user = *itr;
			addSocket(user.ipaddress,atoi(user.portnum));
			if(!((strcmp(user.ipaddress,curNode->ipAddress)==0) & ((atoi(user.portnum) == curNode->portNum))) ){
				sendLeaderMessage(curNode,curServer,ackServer,user);
			}

		}


		updateLeader(curNode);
		curNode->statusServer = NORMAL_OPERATION;
		MESSAGE leaderMsg;
		string msg = "New Leader elected :" + string(curNode->sUserName);
		strcpy(leaderMsg.sContent,msg.c_str());
		leaderMsg.sType = MESSAGE_TYPE_CHAT;
		curNode->consoleQueue.push(leaderMsg);

	}


	return ret;
}

void updateLeader(chat_node* curNode){

	strcpy(curNode->lead.sIpAddress ,curNode->ipAddress);
	curNode->lead.sPort = curNode->portNum;
	strcpy(curNode->lead.sName, curNode->sUserName);

}

int sendElectionMessage(chat_node* curNode, udp_Server* curServer, udp_Server* ackServer, USERINFO user, int numMsgReceived){
	//Setting up Client
	struct sockaddr_in client;
	char msgTosend[16];
	client.sin_addr.s_addr = inet_addr(user.ipaddress);
	client.sin_family = AF_INET;
	client.sin_port = htons(atoi(user.portnum)+2);

	struct timeval tv;
	//TODO To define message types here:- Both sending and receiving

	strcpy(msgTosend,to_string(MESSAGE_TYPE_ELECTION).c_str());
	//Send Election Message to the Nodes with higher port numbers

	int ret = sendto(curServer->get_socket(),&msgTosend,sizeof(msgTosend),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
	if ( ret < 0){
		perror("error while sending the message \n");
	}

	tv.tv_sec = 2;
	if (setsockopt(ackServer->get_socket(), SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		cout<<"Error in this socket\n";
		perror("Error while setting a time constraint on the socket");
	}

	int timeout = 0;
	struct sockaddr_in ackClient;
	char ackMsg[4];
	while(timeout < 2){
		if(ackServer->get_message(ackClient,ackMsg,sizeof(ackMsg))<0){
			perror("Message being resent \n");

			ret = sendto(curServer->get_socket(),&msgTosend,sizeof(msgTosend),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
			if ( ret < 0){
				//Declare that particular client as dead
				perror("error while sending the message \n");
				continue;
			}
			timeout++;
		}
		else{
			cout << "Acknowledgment received\n";
			numMsgReceived++;
			if(strcmp(ackMsg,"ACK")==0)
				break;
		}
	}
	return numMsgReceived;
}

void sendLeaderMessage(chat_node* curNode, udp_Server* curServer, udp_Server* ackServer, USERINFO user){
	//Setting up Client
	struct sockaddr_in client;
	client.sin_addr.s_addr = inet_addr(user.ipaddress);
	client.sin_family = AF_INET;
	client.sin_port = htons(atoi(user.portnum));

	struct timeval tv;
	//TODO To define message types here:- Both sending and receiving
	MESSAGE msgTosend;
	msgTosend.sType = MESSAGE_TYPE_LEADER;
	strcpy(msgTosend.sContent,curNode->ipAddress);
	strcpy(msgTosend.sContent+IP_BUFSIZE,to_string(curNode->portNum).c_str());
	strcpy(msgTosend.sContent+IP_BUFSIZE+PORT_BUFSIZE,curNode->sUserName);
	strcpy(msgTosend.sContent+IP_BUFSIZE+PORT_BUFSIZE
			+USERNAME_BUFSIZE,curNode->rxBytes);

	//Send Election Message to the Nodes with higher port numbers
	int ret = sendto(curServer->get_socket(),&msgTosend,sizeof(MESSAGE),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
	if ( ret < 0){
		perror("error while sending the message \n");
	}

	tv.tv_sec = 2;
	if (setsockopt(ackServer->get_socket(), SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error while setting a time constraint on the socket");
	}

	int timeout = 0;
	struct sockaddr_in ackClient;
	char ackMsg[4];
	while(timeout < 2){
		if(ackServer->get_message(ackClient,ackMsg,sizeof(ackMsg))<0){
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
			cout << "Acknowledgment received\n";

			if(strcmp(ackMsg,"ACK")==0)
				break;
		}
	}

}



