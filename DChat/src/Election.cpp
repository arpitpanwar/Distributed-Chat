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

boost::uuids::random_generator rg;
void sendLeaderMessage(chat_node* curNode, udp_Server* curServer, udp_Server* ackServer, USERINFO user);
vector<string> split(string s, char delim);
int sendElectionMessage(chat_node* curNode, udp_Server* curServer, udp_Server* ackServer, USERINFO user, int numMsgReceived);

int conductElection(chat_node* curNode, udp_Server* curServer, udp_Server* ackServer){
	cout << "Conducting Elections...\n";
	int ret = 0 ;
	int val;
	//Check if your node has the highest port number.
	list<UserInfo>::iterator itr;
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
		//	cout<<"size in conductElection before:"<<curNode->listofUsers.size();

			curNode->listofUsers.erase(itr);

		//	cout<<"size in conductElection after:"<<curNode->listofUsers.size();
			break;
		}
	}

	userList = curNode->getUserList();

	for(itr = userList.begin(); itr != userList.end(); ++itr){
		struct UserInfo user = *itr;

		if((atoi(user.portnum) != curNode->portNum)	|| (strcmp((user.ipaddress),curNode->ipAddress)!= 0)){

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

//	cout << "isHighest:"<<isHighest<<endl;
//	cout <<" numMsg Recevied:"<<numMsgReceived <<endl;
	//TODO Declare Yourself as the leader
	//Checking if the size of listofUsers = 2
	if((curNode->listofUsers.size() == 1) || (isHighest == true) || (numMsgReceived == 0)){
		long seqNumRet;
		cout << "I am the leader\n";

		flushHoldbackQ();

		curNode->bIsLeader = true;
		curNode->mStatusmap.clear();

		curNode->mClientmap.erase(string(curNode->lead.sIpAddress)+":"+to_string(curNode->lead.sPort));
		curNode->listofSockets.clear();
		list<USERINFO> userList = curNode->getUserList();
		for(itr = userList.begin(); itr != userList.end(); ++itr){
			USERINFO user;

			user = *itr;
			addSocket(user.ipaddress,atoi(user.portnum));
			if(!((strcmp(user.ipaddress,curNode->ipAddress)==0) & ((atoi(user.portnum) == curNode->portNum))) ){
			//	cout<<"SENDING LEADER MESSAGE TO :"<<user.username;
				 sendLeaderMessage(curNode,curServer,ackServer,user);

			}

		}


		updateLeader(curNode);
		curNode->lastSeqNum = 0;
		//curNode->statusServer = NORMAL_OPERATION;

		while(!curNode->sendQueue.empty()){
				curNode->sendQueue.pop();
		}

		MESSAGE leaderMsg;
		string msg = "New Leader elected :" + string(curNode->sUserName);
		strcpy(leaderMsg.sContent,msg.c_str());
		leaderMsg.sType = MESSAGE_TYPE_CHAT;
		strcpy(leaderMsg.uuid,boost::lexical_cast<string>(rg()).c_str());

		curNode->consoleQueue.push(leaderMsg);

		map<string,bool>::iterator statusIterator = curNode->mSentMessageMap.begin();
						curNode->lastSeqNum = 0;

				while(statusIterator!=curNode->mSentMessageMap.end()){

					if(statusIterator->second == false){
						MESSAGE msg;
						msg.sType = MESSAGE_TYPE_CHAT_NOSEQ;
						strcpy(msg.sContent,curNode->mMessages[statusIterator->first].c_str());
						strcpy(msg.uuid,boost::lexical_cast<string>(rg()).c_str());
						curNode->mMessages[string(msg.uuid)] = msg.sContent;
						curNode->mSentMessageMap[string(msg.uuid)]=false;
						curNode->consoleQueue.push(msg);
					}

					statusIterator++;
			}
		curNode->mMessages[string(leaderMsg.uuid)] = leaderMsg.sContent;
		curNode->mSentMessageMap[string(leaderMsg.uuid)]=false;
		curNode->electionstatus = NORMAL_OPERATION;
		if(curNode->bIsLeader)
			curNode->statusServer = NORMAL_OPERATION;

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
	tv.tv_usec = 0;
	if (setsockopt(ackServer->get_socket(), SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error while setting a time constraint on the socket while sending Election message");
	}


	int timeout = 0;
	struct sockaddr_in ackClient;
	char ackMsg[ACK_MSGSIZE];
	while(timeout < 2){
		if(ackServer->get_message(ackClient,ackMsg,sizeof(ackMsg))<0){
			perror("Message being resent to : \n");
			cout<<htons(client.sin_port)<<endl;
			ret = sendto(curServer->get_socket(),&msgTosend,sizeof(msgTosend),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
			if ( ret < 0){
				//Declare that particular client as dead
				perror("error while sending the message \n");
				continue;
			}
			timeout++;
		}
		else{
			
			if(strcmp(ackMsg,"ACKELECTION")==0){
			cout << "Acknowledgment received\n";
			numMsgReceived++;
				break;
			}else{
			timeout++;
			}
			
		}
	}
	return numMsgReceived;
}

void sendLeaderMessage(chat_node* curNode, udp_Server* curServer, udp_Server* ackServer, USERINFO user){

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
	strcpy(msgTosend.uuid,boost::lexical_cast<string>(rg()).c_str());

//	cout <<"Sending Ip"<<inet_ntoa(client.sin_addr)<<htons(client.sin_port);
	//Send Election Message to the Nodes with higher port numbers
	int ret = sendto(curServer->get_socket(),&msgTosend,sizeof(MESSAGE),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
	if ( ret < 0){
		perror("error while sending the message \n");
	}
	cout<<"Sending Leader Message to:"<<htons(client.sin_port)<<endl;
	cout<<"Send Leader Message to IP: "<<inet_ntoa(client.sin_addr)<<endl;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	if (setsockopt(ackServer->get_socket(), SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error while setting a time constraint on the socket while sending leader message");
	}

	int timeout = 0;
	struct sockaddr_in ackClient;
	char ackMsg[ACK_MSGSIZE];
	while(timeout < 2){
		if(ackServer->get_message(ackClient,ackMsg,sizeof(ackMsg))<0){
			perror("Leader Message  being resent,ACK not received \n");
			cout<<"RESending Leader Message to:"<<htons(client.sin_port);
			ret = sendto(curServer->get_socket(),&msgTosend,sizeof(MESSAGE),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
			timeout++;
		}
		else{

			if(strcmp(ackMsg,"ACK") == 0){
				break;
			}else{
				timeout++;
			}
		}
	}


}



