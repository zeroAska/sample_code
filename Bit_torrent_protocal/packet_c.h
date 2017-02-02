/*
file:	packet.h
proj:	Ve489 final project
by:		Peng Yuan
on:		July 5, 2015
*/

#ifndef __PACKET_C_H__
#define __PACKET_C_H__

#include <cstring>
#include <string>
#include <vector>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdint.h>
#include "sha1.h"
#include "const_values.h"

struct packet_t
{
	char type[PACKET_CTR_SIZE];
	char data[PACKET_DATA_SIZE];
};


class packet_c
{
public:
	packet_c();
	packet_c(char type, const void * obj); 					// construct a packet from a object
	~packet_c();
	void set_type(const char type);							// set the type of the empty object
	void put_obj(const void * obj, unsigned int length);	// put an object into the data
	void put_data_str(const std::string & str);				// put string into data, a special case for put obj
	unsigned int get_length(void);
	const void * make_packet(void);							// when receive a socket, make an packet from this socket's result object
	//
	void load_packet(const struct packet_t * pkt);			// creat packet class base on packet struct. This is copy, not cast.
	char get_type(void);									// cast the first byte of packet to in order to know how to   parse.
    const void * get_data(void);							// extract data from the coming packet.
private:
	struct packet_t the_packet;
	unsigned int length;
};

#endif
