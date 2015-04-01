#include "defs.h"

using namespace std;
//TODO Decide how to work with type

typedef struct message{

	string sContent;
	string sType;
	string sIpPort;
	long lSequenceNums;

}MESSAGE;

typedef struct leader{
	string sIpAddress;
	string sPort;
	string sName;
}LEADER;

class chat_node{

public:
	chat_node(string userName,int entry);
	~chat_node();
	long getSequenceNumber();
	LEADER getLeader();
	bool getIsLeader();
	string getUserName();
	map<string,string> getClientMap();
	list<string> getPrintQueue();
	list<string> getSendQueue();
	list<MESSAGE> getHoldbackQueue();
	map<string,list<string> > getAckMap();
	MESSAGE getNextMessage();

private :
	bool bIsLeader;
	long lSequencenums;
	string sUserName;
	LEADER lead;
	map<string,string> mClientmap;
	map<string,list<string> > mAckMap;
	list<string> lPrintQueue;
	list<string> lSendQueue;
	list<MESSAGE> mHoldbackQueue;
};


