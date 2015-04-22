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
void addSocket(char ipaddress[], int portNum);

typedef struct message{

	char sContent[MESSAGE_SIZE];
	int sType;
	long lSequenceNums;
	int timestamp;

	bool operator < (const message &rhs)const
	{
	    return rhs.lSequenceNums < lSequenceNums;
	}


}MESSAGE;

typedef struct heartbeat{
	char userName[USERNAME_BUFSIZE];
	int portNum;
	char ipAddress[IP_BUFSIZE];
}HEARTBEAT;

typedef struct ListMessage{
	int numUsers;
	char leaderip[IP_BUFSIZE];
	int  leaderPort;
	long MaxSeqNum;
	char listUsers[MESSAGE_SIZE];
	char leaderName[USERNAME_BUFSIZE];
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
	char rxBytes[RXBYTE_BUFSIZE];

	bool operator == (const UserInfo &rhs)
	{
	    return ((strcmp(rhs.ipaddress,ipaddress)==0) && (strcmp(rhs.portnum,portnum)==0) && (strcmp(rhs.username,username)==0) && (strcmp(rhs.rxBytes,rxBytes)==0));
	}

}USERINFO;



template <typename T>
class PriorityBlockingQueue
{
 public:

  T pop()
  {
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty())
    {
      cond_.wait(mlock);
    }
    auto item = queue_.top();
    queue_.pop();
    mlock.unlock();

    return item;
  }

  T front(){
	  std::unique_lock<std::mutex> mlock(mutex_);
	  while (queue_.empty())
	  {
		  cond_.wait(mlock);
	  }
	  auto item = queue_.top();
	  mlock.unlock();

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
  std::priority_queue<T> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
};




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
    mlock.unlock();

    return item;
  }

  T front(){
	  std::unique_lock<std::mutex> mlock(mutex_);
	  while (queue_.empty())
	  {
		  cond_.wait(mlock);
	  }
	  auto item = queue_.front();
	  mlock.unlock();

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
string exec(char*);
string getRxBytes();

class chat_node{

public:
	chat_node(char userName[],int entry,char ipaddr[] , int port, long lastSeqNum );

	~chat_node();



	bool bIsLeader;
	int  statusServer;
	int electionstatus;
	long lastSeqNum;
	long maxSeqNumseen;
	int entryNum;
	char ipAddress[IP_BUFSIZE];
	char sUserName[USERNAME_BUFSIZE];
	char rxBytes[RXBYTE_BUFSIZE];
	int portNum;
	LEADER lead;
	list<USERINFO> listofUsers;
	map<string,string> mClientmap;
	map<string,double> mStatusmap;
	list<sockaddr_in> listofSockets;
	PriorityBlockingQueue<MESSAGE> holdbackQueue;
	Queue<MESSAGE> consoleQueue;
	Queue<MESSAGE> sendQueue;
	Queue<string> printQueue;


	list<USERINFO> getUserList(){
			list<USERINFO> copyList(listofUsers.begin(),listofUsers.end());
			return copyList;
	}

	list<sockaddr_in> getSocketList(){
		list<sockaddr_in> copySockList(listofSockets.begin(),listofSockets.end());
		return copySockList;
	}

};



void updateLeader(chat_node* curNode);
char* decrypt(char* data);
char* encrypt(char* data);
