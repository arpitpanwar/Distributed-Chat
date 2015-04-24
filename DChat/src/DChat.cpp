
// Author      : Anand Sriramulu
// Version     :
// Copyright   : Your copyright notice
// Description : Main file of the chat service.
//============================================================================

#include "headers/udpserver.h"
#include "headers/defs.h"
#include "headers/chatstructures.h"

using namespace std;

udp_Server* curServer;
udp_Server* heartBeatserver;
udp_Server* ackServer;
chat_node* curNode;

mutex mutex_count;
bool proposedMsgFlag = false;
int proposedTimeout = 0;

void sendMsgrcvAcK(MESSAGE msgTosend);

void addUser(char ipaddress[],int portNum,char username[]){

	string ipPort = ipaddress+string(":")+to_string(portNum);
	string user = username;

	if(curNode->mClientmap.find(ipPort) == curNode->mClientmap.end()){
		curNode->mClientmap[ipPort] = user;
	}
}

//void addSocket(char ipaddress[], int portNum){
//
//	struct sockaddr_in client;
//	client.sin_addr.s_addr = inet_addr(ipaddress);
//	client.sin_family = AF_INET;
//	client.sin_port = htons(portNum);
//	curNode->listofSockets.push_back(client);
//}

void addUserlist(char ipaddress[],int portNum, char username[],char rxsize[]){

	USERINFO temp;
	addUser(ipaddress,portNum,username);
	//addSocket(ipaddress, portNum);
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
			msg =curNode->printQueue.pop();
			cout <<msg<<endl;
			cout.flush();
	}
}

void *recvMsg(void *id){

	int addr_len = sizeof(struct sockaddr);
	int ret;
	int lastSeqNum = 0;
	while(true){
		struct sockaddr_in client;
		MESSAGE msg;
		char ack[4] = "ACK";

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

		if(msg.sType != MESSAGE_TYPE_CHECK){	//Sending Acknowledgment
			client.sin_port = htons(ntohs(client.sin_port)+1);

			ret = sendto(ackServer->get_socket(),&ack,sizeof(ack),0,
					(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
			if ( ret < 0){
				perror("error while sending the message \n");
				continue;
			}
		}

		//If it is a Proposed Message, then update the maximum proposed number
 		if(msg.sType == MESSAGE_TYPE_PROPOSED){
 			//cout << "Proposal Message Received...\n";
 			if(curNode->count == 0){
 				curNode->maxProposed = msg.lSequenceNums;
 				incrementCount();
 			}
 			else if(curNode->count < curNode->listofUsers.size()){
 				if(curNode->maxProposed < msg.lSequenceNums){
 					curNode->maxProposed = msg.lSequenceNums;
 				}
 				//cout << "Proposals Still Pending...\n";
 				incrementCount();
 			}
 			if(curNode->count == curNode->listofUsers.size()){
 				//curNode->count = 0;
 				//cout << "All Proposals Received...\n";
 				proposedMsgFlag = false;
 				proposedTimeout = 0;
 				int max = curNode->maxProposed;
 				curNode->seqNumQueue.push(max);
#ifdef DEBUG
 				cout << "Maximum Proposed:-" << max << "\n";
 				cout.flush();
#endif
 			}
 		}
		//If it is a check message, then send the proposed sequence number
 		if(msg.sType == MESSAGE_TYPE_CHECK){
 			MESSAGE proMsg;
 			curNode->seqProposed = max(curNode->seqProposed,curNode->seqAgreed) + 1;
 			proMsg.sType = MESSAGE_TYPE_PROPOSED;
 			proMsg.lSequenceNums = curNode->seqProposed;
#ifdef DEBUG
 			cout<< "Check Message Received...\n";
 			cout.flush();
#endif
			ret = sendto(curServer->get_socket(),&proMsg,sizeof(MESSAGE),0,
					(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
			if ( ret < 0){
				perror("error while sending the message \n");
				continue;
			}
 		}

 		else if((msg.sType == MESSAGE_TYPE_JOIN) || (msg.sType == MESSAGE_TYPE_UPDATE)){
 			curNode->consoleQueue.push(msg);
		}
		else if(msg.sType == MESSAGE_TYPE_CHAT){
			//Updating the Agreed Sequence number of the current node
			curNode->seqAgreed = max(curNode->seqAgreed,msg.lSequenceNums);
#ifdef DEBUG
			cout << "Agreed:-" << curNode->seqAgreed << "\n";
			cout.flush();
#endif
			string content = msg.sContent;
			string key = ip+string(":")+to_string(port);
			if(curNode->mClientmap.find(key) != curNode->mClientmap.end()){
				content = curNode->mClientmap[key]+string(":: ")+content;
				strcpy(&msg.sContent[0],content.c_str());
			}
			if(lastSeqNum!= msg.lSequenceNums){
				if(lastSeqNum == 0 )
					curNode->seqlastSeen = msg.lSequenceNums -1;

					lastSeqNum = msg.lSequenceNums;
			}
#ifdef DEBUG
			cout << "Last Seen:-" << curNode->seqlastSeen << "\n";
			cout.flush();
#endif
			curNode->holdbackQueue.push(msg);
		}
		else if(msg.sType == MESSAGE_TYPE_NOTICE){
			curNode->printQueue.push(msg.sContent);
		}
		else if(msg.sType == MESSAGE_TYPE_REMOVE_USER){

			{
				string content = msg.sContent;
			//	cout<<"Content is "<<content<<endl;
				vector<string> tokens = split(content,':');

				list<USERINFO> userList = curNode->getUserList();
				list<USERINFO>::iterator itr  = userList.begin();

				while(itr!=userList.end()){
					USERINFO user = *itr;

					if((strcmp(user.ipaddress,tokens[0].c_str())==0) && (strcmp(user.portnum,tokens[1].c_str())==0)){

			//			cout<<"size in MESSAGE_TYPE_REMOVE_USER  before:"<<curNode->listofUsers.size();

						curNode->listofUsers.remove(user);

				//		cout<<"size in MESSAGE_TYPE_REMOVE_USER  after:"<<curNode->listofUsers.size();


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


		}
#ifdef PRINT
		cout << "\nMessage being received :: " ;
		cout << msg.sContent;
		std::cout.flush();
#endif

	}
}

void *sendMsg(void *id){
	int addr_len = sizeof(struct sockaddr);
	int seqNum = 0;


	while(true){

			MESSAGE msgTosend;
			//Blocked on pop
			msgTosend = curNode->sendQueue.pop();
			if(msgTosend.sType == MESSAGE_TYPE_CHECK){
				proposedMsgFlag = true;
				proposedTimeout = 0;
			}
			if(msgTosend.sType == MESSAGE_TYPE_CHAT){
				int maxProposedValue = curNode->seqNumQueue.pop();
				msgTosend.lSequenceNums = maxProposedValue;
			}
#ifdef DEBUG
			cout << "Send Message: Send Queue not empty\n";
			cout << curSentmsg.message;
#endif
			sendMsgrcvAcK(msgTosend);
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
	int num, ret;
	char ip[IP_BUFSIZE];
	char portNum[PORT_BUFSIZE];
	char username[USERNAME_BUFSIZE];
	char rxsize[RXBYTE_BUFSIZE];
	int port;
	LISTMSG msgTosend;
	MESSAGE updateMsg;
	struct sockaddr_in client;

	memcpy(ip,msg,IP_BUFSIZE);
	memcpy(portNum,msg+IP_BUFSIZE,PORT_BUFSIZE);
	memcpy(username,msg+IP_BUFSIZE+PORT_BUFSIZE,USERNAME_BUFSIZE);
	memcpy(rxsize,msg+IP_BUFSIZE+PORT_BUFSIZE+USERNAME_BUFSIZE,RXBYTE_BUFSIZE);

	sscanf(portNum, "%d", &port);

	client.sin_addr.s_addr = inet_addr(ip);
	client.sin_family = AF_INET;
	sscanf(portNum, "%d", &port);
	client.sin_port = htons(port);

	num = populatelistofUsers(msgTosend.listUsers);

	msgTosend.numUsers = num;

	ret = sendto(curServer->get_socket(),&msgTosend,sizeof(LISTMSG),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
	if ( ret < 0){
		perror("error while sending the List Message \n");

	}

	updateMsg.sType = MESSAGE_TYPE_UPDATE;
	string cont = ip+string(":")+portNum+string(":")+username+string(":")+rxsize;
	strcpy(updateMsg.sContent,cont.c_str());

	sendMsgrcvAcK(updateMsg);

//	MESSAGE update;
//	MESSAGE checkMsg;
//	checkMsg.sType = MESSAGE_TYPE_CHECK;
//	initializeCount();
//	update.sType = MESSAGE_TYPE_CHAT;
//	strcpy(update.sContent, string(string("NOTICE:: ")+string(username)+string(" joined on:: ")+string(ip)+string(":")+to_string(port)).c_str());
//
//	curNode->consoleQueue.push(checkMsg);
//	curNode->consoleQueue.push(update);

	MESSAGE update;
	update.sType = MESSAGE_TYPE_NOTICE;
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

		if(curMsg.sType == MESSAGE_TYPE_JOIN){

			sendlist(curMsg.sContent);

		}
		else if(curMsg.sType == MESSAGE_TYPE_UPDATE){
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
		}
		else{
					curNode->sendQueue.push(curMsg);
		}
	}
	return (void*)NULL;
}

void *holdbackThread(void *id){
	string printMsg;
	while (true){

			MESSAGE curMsg;
			curMsg = curNode->holdbackQueue.front();
			if(curMsg.lSequenceNums == curNode->seqlastSeen + 1){
				curNode->seqlastSeen = curNode->seqlastSeen + 1;
				curNode->holdbackQueue.pop();
				printMsg = string(curMsg.sContent);
				curNode->printQueue.push(printMsg);
			}
	}

}
void *timeoutThread(void *id){
	while(true){
		if(proposedMsgFlag){
			proposedTimeout++;
			if(proposedTimeout == 5){
				proposedMsgFlag = false;
				proposedTimeout = 0;
				curNode->seqNumQueue.push(curNode->maxProposed);
			}
			sleep(1);
		}
	}
	return (void*)NULL;
}
void *heartbeatThread(void *id){
	struct sockaddr_in client;
	int ret;
	int addr_len = sizeof(struct sockaddr);
	struct timeval tv;
	char sendBeat[16],recvBeat[16];
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
		}
		else{
				string received = recvBeat;
				if(received.compare(BEAT)==0){
					timeval curr;
					gettimeofday(&curr,NULL);
					string ip = string(inet_ntoa(client.sin_addr));
					int port = ntohs(client.sin_port);
					string key = ip+string(":")+to_string(port-2);

					curNode->mStatusmap[key] = curr.tv_sec;
			}
		}
	}
}

void* heartbeatSend(void *id){
		int ret;
		int addr_len = sizeof(struct sockaddr);
		char sendBeat[16];
		while(true){

				for(list<USERINFO>::const_iterator iterator = curNode->listofUsers.begin(); iterator != curNode->listofUsers.end(); ++iterator){

					struct UserInfo user  = *iterator;
#ifdef DEBUG
					cout << user.username;
					cout.flush();
					sleep(2);
#endif

					if(strcmp(user.username, curNode->sUserName) != 0){
#ifdef DEBUG
						cout << "Sending Beat...\n";
						cout << curNode->listofUsers.size() << "\n";
						cout.flush();
#endif
						//Setting up Client
						struct sockaddr_in client;
						client.sin_addr.s_addr = inet_addr(user.ipaddress);
						client.sin_family = AF_INET;
						client.sin_port = htons(atoi(user.portnum) + 2);

						strcpy(sendBeat,BEAT);

						ret = sendto(heartBeatserver->get_socket(),&sendBeat,sizeof(sendBeat),0,(struct sockaddr *)&client,(socklen_t)addr_len);
						if ( ret < 0){
							perror("error while sending the heartbeat signal \n");
						}
					}
				}

		map<string,double>::iterator it = curNode->mStatusmap.begin();
		timeval start;
		gettimeofday(&start,NULL);
		//double curTime = clock()/CLOCKS_PER_SEC;
		while(it != curNode->mStatusmap.end()){
				if((  start.tv_sec - it->second) >=8){

					list<USERINFO> userList = curNode->getUserList();
					list<USERINFO>::iterator itr = userList.begin();
					MESSAGE removeMsg,removeChat;
					string ipport = it->first;
					map<string,string>::iterator mapIt = curNode->mClientmap.find(ipport);

					if(mapIt!=curNode->mClientmap.end()){
						curNode->mClientmap.erase(mapIt);
					}

					vector<string> tokens = split(ipport,':');


					while(itr!=userList.end()){
						USERINFO user = *itr;
						if((strcmp(user.ipaddress,tokens[0].c_str())==0) && (strcmp(user.portnum,tokens[1].c_str())==0)){

			//				cout<<"size in Client dead  before:"<<curNode->listofUsers.size();

							curNode->listofUsers.remove(user);

			//				cout<<"size in Client dead  after:"<<curNode->listofUsers.size();

							removeMsg.sType = MESSAGE_TYPE_REMOVE_USER;
							strcpy(removeMsg.sContent,it->first.c_str());
							curNode->sendQueue.push(removeMsg);

							string remove = "Notice "+string(user.username)+" left the group or crashed";
							removeChat.sType=MESSAGE_TYPE_NOTICE;
							strcpy(removeChat.sContent,remove.c_str());

							curNode->sendQueue.push(removeChat);

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
				it++;
			}
		sleep(5);
		}

}

void populatesocketClient(char userList[],int numUser){

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

void sendMsgrcvAcK(MESSAGE msgTosend){
	int ret;
	struct timeval tv;

	for(list<USERINFO>::const_iterator iterator = curNode->listofUsers.begin(); iterator != curNode->listofUsers.end(); ++iterator){
			struct UserInfo user  = *iterator;

			//Setting up Client
			struct sockaddr_in client;
			client.sin_addr.s_addr = inet_addr(user.ipaddress);
			client.sin_family = AF_INET;
			client.sin_port = htons(atoi(user.portnum));

			ret = sendto(curServer->get_socket(),&msgTosend,sizeof(MESSAGE),0,(struct sockaddr *)&client,(socklen_t)sizeof(struct sockaddr));
			if ( ret < 0){
				perror("error while sending the message \n");
				continue;
			}
			if(msgTosend.sType != MESSAGE_TYPE_CHECK){
				tv.tv_sec = 2;
				if (setsockopt(ackServer->get_socket(), SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
					//perror("Error while setting a time constraint on the socket");
				}
				{
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

							if(strcmp(ackMsg,"ACK")==0)
								break;
						}
					}
				}
			}
#ifdef PRINT
			cout << "\nMessage being Sent :: ";
			cout << msgTosend.sContent;
#endif
		}
}

void printUsers(){
	cout << "Succeeded, current users:\n";
	for(list<USERINFO>::const_iterator iterator = curNode->listofUsers.begin(); iterator != curNode->listofUsers.end(); ++iterator){
		struct UserInfo user  = *iterator;
		cout << user.username << " " << user.ipaddress << ':' << user.portnum << "\n";
		cout.flush();
	}
}

void initializeCount(){
	mutex_count.lock();
	curNode->count = 0;
    curNode->maxProposed = 0;
    mutex_count.unlock();
}

void incrementCount(){
	mutex_count.lock();
	curNode->count = curNode->count + 1;
	mutex_count.unlock();
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
	if(pthread_create(&threads[7],NULL,timeoutThread,NULL)){
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
		cout << "Joining an existing chat \n";
	}
	//Trying to get an open port
	portNum = getOpenPort();
	cout<<"Port is:"<<portNum<<endl;
	strcpy(username,argv[1]);
	strcpy(ipaddress,findip().c_str());
	string bytes = getRxBytes();
	strcpy(rxsize,bytes.c_str());

	//Starting Message and the Acknowledgment server on different ports
	curServer = new udp_Server(ipaddress,portNum);
	ackServer = new udp_Server(ipaddress,portNum+1);
	heartBeatserver = new udp_Server(ipaddress,portNum+2);
	curNode = new chat_node(username,entry,ipaddress,portNum);

	if(isSeq){
		addUserlist(ipaddress,portNum,username,rxsize);
		//addSocket(ipaddress,portNum);
	}
	else{
		addUserlist(ipaddress,portNum,username,rxsize);
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
		joinMsg.sType = MESSAGE_TYPE_JOIN;

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
			perror("error while receiving the message \n");
			//continue;
		}
		char ack[4];
		seqClient.sin_port = htons(ntohs(seqClient.sin_port )+1);
		ret = sendto(ackServer->get_socket(),&ack,sizeof(ack),0,
						(struct sockaddr *)&seqClient,(socklen_t)sizeof(struct sockaddr));
		if ( ret < 0){
			perror("error while sending the acknowledgment \n");
		}

		populatesocketClient(userListMsg.listUsers,userListMsg.numUsers);
		printUsers();
	}

	if(create_threads(threads)){
		perror("Error Creating threads \n");
		exit(1);
	}


	while(true){
		MESSAGE checkMsg;
		MESSAGE chatMsg;
		string inpString;
		getline(cin,inpString);

		checkMsg.sType = MESSAGE_TYPE_CHECK;
		initializeCount();
		chatMsg.sType = MESSAGE_TYPE_CHAT;
		strcpy(chatMsg.sContent,inpString.c_str());

		curNode->consoleQueue.push(checkMsg);
		curNode->consoleQueue.push(chatMsg);

	}

	return 0;
}
