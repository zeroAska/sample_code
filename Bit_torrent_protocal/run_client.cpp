#include "client_c.h"
#include "const_values.h"
#include <iostream>


 using namespace std;
 int main(int argc, char ** argv)
 {
	
 	/*connection_info_t connection;
 	char * ip = argv[1];
 	bzero(connection.ip,sizeof(connection.ip));
 	memcpy(connection.ip,ip,sizeof(ip));
 	connection.port_num = (uint32_t)atoi(argv[1]);
	cout<<"[client] will start client..."<<endl;*/
	client_c client;
	//client.set_server(connection);
	client.interface();
	return 0;
}
