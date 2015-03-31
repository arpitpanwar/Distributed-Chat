#include <iostream>
#include<cstdlib>

using namespace std;

string findip(){
	FILE *stream;
	string data;
	char ipaddress[16];
	char buffer[1000];
	stream = popen("/sbin/ifconfig | grep inet ","r");

	if (stream) {
		while (!feof(stream))
			if (fgets(buffer, 1000, stream) != NULL){
				data.append(buffer);
			}
		pclose(stream);
	}
	//erase two lines because the ipaddress lies in the third line
	data.erase(0, data.find("\n") + 1);
	data.erase(0, data.find("\n") + 1);
	//TODO ::changing the size of ipaddress to 16 is leading to unexpected results
	memcpy(ipaddress,data.c_str()+20,15);
	return ipaddress;



}
