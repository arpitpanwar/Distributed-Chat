#include "defs.h"
#include <mutex>
#include <condition_variable>
#include <deque>

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

typedef struct sendmessage{
	string message;
	list<sockaddr_in>;
};

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


template <typename T>
class queue
{
private:
    std::mutex              d_mutex;
    std::condition_variable d_condition;
    std::deque<T>           d_queue;
public:
    void push(T const& value) {
        {
            std::unique_lock<std::mutex> lock(this->d_mutex);
            d_queue.push_front(value);
        }
        this->d_condition.notify_one();
    }
    T pop() {
        std::unique_lock<std::mutex> lock(this->d_mutex);
        this->d_condition.wait(lock, [this]{ return !this->d_queue.empty(); });
        T rc(std::move(this->d_queue.back()));
        this->d_queue.pop_back();
        return rc;
    }
};

queue<message> holdbackQueue;
queue<message> chatQueue;
queue<message> statusQueue;
queue<sendmessage> sendQueue;
queue<message> ackQueue;
queue<string> printQueue;


