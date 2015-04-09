#include "defs.h"
#include <mutex>
#include <condition_variable>
#include <deque>
#include <queue>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
using namespace std;
//TODO Decide how to work with type


typedef struct message{

	char sContent[2048];
	int sType;
	int lSequenceNums;
//	char userName[32];
//	char ipaddress[16];
//	int portNum;

}MESSAGE;

typedef struct ListMessage{
	int numUsers;
	char leaderip[32];
	int  leaderPort;
	char listUsers[2048];
}LISTMSG;

typedef struct leader{
	char sIpAddress[32];
	int sPort;
	char sName[32];
}LEADER;

typedef struct ipPort{
	char ipaddress[16];
	char portnum[4];
}IPPORT;

template <typename T>
class Queue
{
 public:

  T pop()
  {
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty())
    {
      cond_.wait(mlock);
    }
    auto item = queue_.front();
    queue_.pop();
    return item;
  }

  void pop(T& item)
  {
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty())
    {
      cond_.wait(mlock);
    }
    item = queue_.front();
    queue_.pop();
  }
  bool empty()
      {
	  	  std::unique_lock<std::mutex> mlock(mutex_);
          return queue_.empty();
      }

  void push(const T& item)
  {
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push(item);
    mlock.unlock();
    cond_.notify_one();
  }

  void push(T&& item)
  {
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push(std::move(item));
    mlock.unlock();
    cond_.notify_one();
  }

 private:
  std::queue<T> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
};


class chat_node{

public:
	chat_node(char userName[],int entry,char ipaddr[] , int port  );

	~chat_node();
	bool bIsLeader;
//	long lSequencenums;
	int entryNum;
	char ipAddress[16];
	char sUserName[32];
	int portNum;
	LEADER lead;
	list<IPPORT> listofUsers;
//	map<string,string> mClientmap;
	list<sockaddr_in> listofSockets;
	Queue<message> holdbackQueue;
	Queue<message> chatQueue;
	Queue<message> statusQueue;
	Queue<message> consoleQueue;
	Queue<message> sendQueue;
	Queue<message> ackQueue;
	Queue<string> printQueue;
};





