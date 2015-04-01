#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string>
typedef struct user_details{
	struct sockaddr_in f_addrclient;



}USER_DETAILS;

namespace chat_Server{

//class chatServer_runtime_error : public std::runtime_error
//{
//public:
//	chatServer_runtime_error(const char *w) : std::runtime_error(w) {}
//};

class udp_Server{
public :
	//Constructor of the UDP server being created
	udp_Server( char *ipAddr, int port );
	//Destructor of the UDP server
	~udp_Server();

	int get_socket() const;
	int get_portNum() const;
	std::string get_addr() const;
	int  get_message(char *msg, size_t max_size ,int portnum);
	int  send_message(char *msg , size_t max_size , int portnum);


private :
	int f_socket;
	int f_port;
	std::string f_addr;
	struct sockaddr_in f_addrserver;
	 USER_DETAILS f_user[15];

};
}
