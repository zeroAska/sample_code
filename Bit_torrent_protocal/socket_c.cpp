/*
file:    socket.cpp
proj:    Ve489 final project
by:      Peng Yuan
on:      July 5, 2015
*/

#include <string.h>
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <stdio.h>
#include "packet_c.h"
#include "socket_c.h"
#include "const_values.h"


using namespace std;

socket_c::socket_c() // for listening (server) side
{
	this -> socket_desc = socket(AF_INET, SOCK_STREAM, 0);
}

socket_c::socket_c(int client_sock) // for listening (server) side
{
	this -> client_sock = client_sock;
}


socket_c::socket_c(int socket_desc, char k) // for listening (server) side
{
	this -> socket_desc = socket_desc;
}

socket_c::~socket_c()
{
	close(client_sock);	// close socket when object distroyed
}

socket_c::socket_c(connection_info_t & receiver)
{
	this -> port = receiver.port_num;	// set port number to be connected, ie port for the remote PC
	strcpy(this -> ip, receiver.ip);	// set the IP to be connected
	this -> client_sock = socket(AF_INET, SOCK_STREAM, 0);	// create socket
	if (this -> client_sock == -1)
    {
        printf("Could not create socket\n");
    }
    //printf("Socket created\n");
}

bool socket_c::my_connect(void)
{
	this -> server.sin_addr.s_addr = inet_addr(this -> ip);	// prepare to connect
	this -> server.sin_family = AF_INET;
	this -> server.sin_port = htons( this -> port );

	if (connect(this -> client_sock , (struct sockaddr *)&server , sizeof(server)) < 0) // connext
	{
		printf("connect failed. Error\n");
		return false;
	}
	//printf("Connected at\n\n");
	//std::cout << "Connected to port " << this -> port << " on " << this -> ip << std::endl;
	return true;
}

int socket_c::my_connect_ret(void)
{
	this -> server.sin_addr.s_addr = inet_addr(this -> ip);	// prepare to connect
	this -> server.sin_family = AF_INET;
	this -> server.sin_port = htons( this -> port );

	if (connect(this -> client_sock , (struct sockaddr *)&server , sizeof(server)) < 0) // connext
	{
		printf("connect failed. Error\n");
		return 0;
	}
	//printf("Connected at\n\n");
	//std::cout << "Connected to port " << this -> port << " on " << this -> ip << std::endl;
	return this -> client_sock;
}

bool socket_c::my_send(packet_c * packet) // general send, send one packet
{
	//printf("sending to skt %d\n", this -> client_sock);
	if( send(this -> client_sock , packet->make_packet() , PACKET_SIZE , MSG_WAITALL) < 0) // TODO: MSG_WAITALL? not sure for sending
	{
		printf("Send failed\n");
		return false;
	}
	// write(this -> client_sock , packet.make_packet() , packet.get_length());
	//printf("Sent!\n");
	return true;
}

bool socket_c::listen_and_accept(int port_num) // for client listening side, can return client_sock
{
	if (listen_to_port(port_num) == false)
	{
		cerr<<"[Socket]listen failed!"<<endl;
		return false;
	}
	//Accept and incoming connection
	printf("[Socket] [Listen] Waiting for incoming connections...\n");
	c = sizeof(struct sockaddr_in);

	//accept connection from an incoming client
	this -> client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

	if (this -> client_sock < 0)
	{
		printf("accept failed\n");
		return false;
	}
	printf("[Socket] [Listen] Connection accepted\n");

	return true;
}

bool socket_c::listen_to_port(int port)
{
	this -> server.sin_family = AF_INET;	// prepare for listening
	this -> server.sin_addr.s_addr = INADDR_ANY;
	this -> server.sin_port = htons( port );
	int error_msg;
	if( (error_msg = bind(socket_desc,(struct sockaddr *)&server , sizeof(server)))  < 0)
	{
		printf("[Socket] [Listen] bind failed. Error: %d\n",error_msg);
		return false;
	}
	printf("[Socket] [Listen] bind done\n");
	listen(socket_desc , SOMAXCONN);
	return true;
}
/*
bool socket_c::accept_one_socket()
{
	c = sizeof(struct sockaddr_in);

	//accept connection from an incoming client
	this -> client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);	// blocked here, until connected

	if (this -> client_sock < 0)
	{
		cerr<<"accept failed\n";
		return false;
	}
	cout<<"Connection accepted\n";

	return true;
}
*/
int socket_c::get_dest_host(connection_info_t & remote_info)
{
		
	memcpy(remote_info.ip, inet_ntoa(server.sin_addr),sizeof(remote_info.ip));
	remote_info.port_num =  (uint32_t) ntohs(server.sin_port);
	return 0;
}


int socket_c::get_client_host(connection_info_t & remote_info)
{

	memcpy(remote_info.ip, inet_ntoa(client.sin_addr),sizeof(remote_info.ip));
	remote_info.port_num =  (uint32_t) ntohs(client.sin_port);
/*	
	struct sockaddr tmp;
	c = sizeof(struct sockaddr_in);

	int ret = getpeername(client_sock, &tmp, (socklen_t *)&c);
	if (ret != 0)
	{
		printf("get remote host name and port fails!");
		return -1;
	}

	struct sockaddr_in* sin;

	if (tmp.sa_family == AF_INET)
	{
		sin = (struct sockaddr_in *) &tmp;
		ip = inet_ntoa(sin->sin_addr);
	}
	else
	{
		printf("sock sa_family is not AF_INET!\n");
		return -1;
	}

	// update client
*/	
	return 0;
	
}

bool socket_c::my_listen(int port_num) // for listening (server) side, can return client_sock
{
	this -> server.sin_family = AF_INET;	// prepare for listening

	char ip_buf[20];
	get_my_ip(ip_buf);
	this -> server.sin_addr.s_addr = INADDR_ANY; //inet_addr(ip_buf);
	this -> server.sin_port = htons( port_num );
	int msg = 0;
	if( (msg = bind(socket_desc,(struct sockaddr *) &server , sizeof(server) ) ) < 0)	// bind the port

	{
		printf("[Socket] bind failed. Error msg is %d\n",msg);
		return false;
	}

	printf("[socket] bind done, listening to %s:%d\n", ip_buf, port_num);

	listen(socket_desc , SOMAXCONN);
	//listen(socket_desc , 3);	// start to listen

	return true;
}


int socket_c::my_accept(void)
{
	//Accept and incoming connection
	printf("[socket] [accept] Waiting for incoming connections...\n");
	c = sizeof(struct sockaddr_in);

	//accept connection from an incoming client
	this -> client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);	// blocked here, until connected
	if (this -> client_sock < 0)
	{
		cerr<<"[socekt] accept failed\n";
		return this->client_sock;
	}
	printf("[socket] Connection accepted\n");
	return this -> client_sock;

}

int socket_c::receive_single_pkt(packet_t & incoming_pkt) // read one packet, and return via <& incoming_pkt>
{ 
	char * buffer = new char[PACKET_SIZE];
	int read_size = recv(this -> client_sock , buffer , PACKET_SIZE , MSG_WAITALL);	// wait for a full packet
	//printf("receive from skt %d\n", this -> client_sock);
	if (read_size == PACKET_SIZE)
	{
		memcpy( & incoming_pkt, buffer, PACKET_SIZE);
		delete[] buffer;
		return read_size;
	}
	//printf("incompleted packet detected with %d or disconnected!\n", read_size); // literally
	delete[] buffer;
	return read_size;
}

void get_my_ip(char * my_ip)
{
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "enp0s25", IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	/* display result */
	strcpy(my_ip, inet_ntoa(((struct sockaddr_in *) & ifr.ifr_addr) -> sin_addr));
	//printf("my ip: %s\n", my_ip);
	return;
}

void mac_eth0(char MAC_str[13])
{
    #define HWADDR_len 6
    /*int s,i;
    struct ifreq ifr;
    s = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, "eth0");
    ioctl(s, SIOCGIFHWADDR, &ifr);
    for (i=0; i<HWADDR_len; i++)

        sprintf(&MAC_str[i*2],"MAC: %02X \n",((unsigned char*)ifr.ifr_hwaddr.sa_data)[i]);
    MAC_str[12]='\0';
}

        sprintf(&MAC_str[i*2],"%02X",((unsigned char*)ifr.ifr_hwaddr.sa_data)[i]);
    */
    //MAC_str[12]='\0';
    //sprintf("?mac? %s\n", );
    sprintf(MAC_str,"28:cf:e9:4c:3b:1d");
}

