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
	chat_node(string userName,int entry,string ipaddr , string port  );

	~chat_node();
	bool bIsLeader;
	long lSequencenums;
	int entryNum;
	string sUserName;
	LEADER lead;
	map<string,string> mClientmap;
	map<string,list<string> > mAckMap;
	list<string> lPrintQueue;
	list<string> lSendQueue;
	list<MESSAGE> mHoldbackQueue;
};


