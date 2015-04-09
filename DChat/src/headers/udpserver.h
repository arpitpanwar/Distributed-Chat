#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string>
std::string findip();
class udp_Server{
public :
	//Constructor of the UDP server being created
	udp_Server(const char *ipAddr, int port );
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


};

