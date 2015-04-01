#include "defs.h"

using namespace std;

class chat_node{

public:
	chat_node(string userName,int entry);
	~chat_node();
	long getSequenceNumber();
	leader getLeader();
	bool getIsLeader();
	string getUserName();
	map<string,string> getClientMap();
	list<string> getPrintQueue();
	list<string> getSendQueue();
	list<message> getHoldbackQueue();
	map<string,list<string>> getAckMap();
	message getNextMessage();

private :
	bool bIsLeader;
	long lSequencenums;
	string sUserName;
	leader lead;
	map<string,string> mClientmap;
	map<string,list<string>> mAckMap;
	list<string> lPrintQueue;
	list<string> lSendQueue;
	list<message> mHoldbackQueue;
};

//TODO Decide how to work with type

struct message{

	string sContent;
	string sType;
	string sIpPort;
	long lSequenceNums;

};

struct leader{
	string sIpAddress;
	string sPort;
	string sName;
}
