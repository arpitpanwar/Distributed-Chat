#include "defs.h"
#include <mutex>
#include <condition_variable>
#include <deque>
#include <queue>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<iostream>
using namespace std;
//TODO Decide how to work with type


typedef struct message{

	char sContent[MESSAGE_SIZE];
	int sType;
	int lSequenceNums;
	int timestamp;

}MESSAGE;

typedef struct heartbeat{
	char userName[USERNAME_BUFSIZE];
	int portNum;
	char ipAddress[IP_BUFSIZE];
}HEARTBEAT;

typedef struct ListMessage{
	int numUsers;
	char leaderip[USERNAME_BUFSIZE];
	int  leaderPort;
	char listUsers[MESSAGE_SIZE];
}LISTMSG;

typedef struct leader{
	char sIpAddress[IP_BUFSIZE];
	int sPort;
	char sName[USERNAME_BUFSIZE];
}LEADER;

typedef struct UserInfo{
	char ipaddress[IP_BUFSIZE];
	char portnum[PORT_BUFSIZE];
	char username[USERNAME_BUFSIZE];
}USERINFO;

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

int getOpenPort();
vector<string> split(string,char);

class chat_node{

public:
	chat_node(char userName[],int entry,char ipaddr[] , int port  );

	~chat_node();
	bool bIsLeader;
//	long lSequencenums;
	int entryNum;
	char ipAddress[IP_BUFSIZE];
	char sUserName[USERNAME_BUFSIZE];
	int portNum;
	LEADER lead;
	list<USERINFO> listofUsers;
	map<string,string> mClientmap;
	list<sockaddr_in> listofSockets;
	Queue<message> holdbackQueue;
//	Queue<message> chatQueue;
//	Queue<message> statusQueue;
	Queue<message> consoleQueue;
	Queue<message> sendQueue;
//	Queue<message> ackQueue;
	Queue<string> printQueue;
};





