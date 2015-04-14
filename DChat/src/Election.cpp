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

vector<string> split(string s, char delim);
int sendElectionMessage(chat_node* curNode, udp_Server* curServer, udp_Server* ackServer, USERINFO user, int numMsgReceived);

void conductElection(chat_node* curNode, udp_Server* curServer, udp_Server* ackServer){
	cout << "Conducting Elections...\n";

	//Check if your node has the highest port number.
	list<UserInfo>::iterator itr;
	bool isHighest = true;
	int numMsgReceived = 0;

#ifdef DEBUG
	cout << curNode->listofUsers.size();
	cout << curNode->listofSockets.size();
#endif

	for(itr = curNode->listofUsers.begin(); itr != curNode->listofUsers.end(); ++itr){
		struct UserInfo user = *itr;

		if(curNode->portNum < atoi(user.portnum)){
			isHighest = false;
			numMsgReceived = sendElectionMessage(curNode, curServer, ackServer, user, numMsgReceived);
		}
		else if(curNode->portNum == atoi(user.portnum)){
			//Check IP address here
			isHighest = false;
			vector<string> ip_user = split(user.ipaddress, '.');
			vector<string> ip_curNode = split(curNode->ipAddress, '.');

			//Comparing the two IP address starting from the last octet
			for(int i=3; i>=0; i--){
				if(ip_curNode[i] < ip_user[i]){
					numMsgReceived = sendElectionMessage(curNode, curServer, ackServer, user, numMsgReceived);
					break;
				}
			}
		}
	}
	//TODO Declare Yourself as the leader
	//Checking if the size of listofUsers = 2
	if((curNode->listofUsers.size() == 1) || (isHighest == true) || (numMsgReceived == 0)){
		cout << "I am the leader\n";
	}
}

int sendElectionMessage(chat_node* curNode, udp_Server* curServer, udp_Server* ackServer, USERINFO user, int numMsgReceived){
	//Setting up Client
	struct sockaddr_in client;
	client.sin_addr.s_addr = inet_addr(user.ipaddress);
	client.sin_family = AF_INET;
	client.sin_port = htons(atoi(user.portnum));

	struct timeval tv;
	//TODO To define message types here:- Both sending and receiving
	MESSAGE msgTosend;
	msgTosend.sType = MESSAGE_TYPE_ELECTION;
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
			numMsgReceived++;
			if(strcmp(ackMsg,"ACK")==0)
				break;
		}
	}
	return numMsgReceived;
}



