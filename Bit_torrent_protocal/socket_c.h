/*
file:    socket.h
proj:    Ve489 final project
by:      Peng Yuan
on:      July 5, 2015
*/

#ifndef __SOCKET_C_H__
#define __SOCKET_C_H__

#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdint.h>
#include "sha1.h"
#include "packet_c.h"
#include "const_values.h"

class socket_c
{
public:
	socket_c();
	socket_c(int client_sock);
	socket_c(int socket_desc, char k);
	~socket_c();
	// out
	socket_c(connection_info_t & receiver);	// connect to <receiver> constructor
	bool my_connect(void);	// connect to <receiver>
	int my_connect_ret(void);
	bool my_send(packet_c * packet);	// general send, send one packet
	// in
	bool listen_to_port(int port); // server side listen
	int get_dest_host(connection_info_t & remote_info);
	bool listen_and_accept(int port_num);   // open the port for listening
	int receive_single_pkt(packet_t & incoming_pkt);                                   // read data from the pipe of this socket, and return a packet. Also, send a confirm to the sender 
	int get_client_host(connection_info_t & remote_info); // tracker and server: when one client connect with them, the socket will be used to obtain the client's ip and port
	//bool listen_and_accept(int port_num);   // for listening (server) side, can return client_sock if needed
	int my_accept(void);
	bool my_listen(int port_num);

private:
	int port;
	char ip[20];
	int c, socket_desc, client_sock;
	struct sockaddr_in client, server;
};

void get_my_ip(char * my_ip);
void mac_eth0(char MAC_str[13]);

#endif
