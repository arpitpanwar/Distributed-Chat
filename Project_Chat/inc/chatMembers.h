#include<stdlib.h>

#define MAX_USERS 25

namespace chatMembers {

class chat_members{

public:
	chat_members(std::string userName,int entry);
	~chat_members();
private :
	std::string userName;
	int entryNum;
};







}
