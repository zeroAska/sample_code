/*
file:	packet.cpp
proj:	Ve489 final project
by:		Peng Yuan
on:		July 5, 2015
*/

#include <string>
#include <cstring>
#include <unistd.h> //write
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "const_values.h"
#include "packet_c.h"

packet_c::packet_c()
{}

packet_c::packet_c(char type, const void * obj)  // construct a packet from a object
{
	*(this->the_packet.type) = type;
	memcpy(this->the_packet.data, obj, PACKET_DATA_SIZE);
} 

packet_c::~packet_c()
{}

void packet_c::set_type(const char type) // set the type of the empty object
{
	*(this->the_packet.type) = type;
} 

void packet_c::put_obj(const void * obj, unsigned int length)  // put an object into the data
{
	memcpy(this->the_packet.data, obj, length);
	this->length = length;
}

void packet_c::put_data_str(const std::string & str)
{
	strcpy(the_packet.data, str.c_str());
}

const void * packet_c::make_packet(void) // when receive a socket, make an packet from this socket's result object
{
	return (void *) & this->the_packet;	// return const pointer to a private member
}

void packet_c::load_packet(const packet_t * pkt)	// creat packet class base on packet struct
{
	memcpy(& this->the_packet, pkt, PACKET_SIZE);
}

char packet_c::get_type(void) // cast the first byte of packet to in order to know how to parse.
{
	return *(this -> the_packet.type);
}

const void * packet_c::get_data(void) // extract data from the coming packet.
{
	return (void *) & this->the_packet.data;
}

unsigned int packet_c::get_length(void)
{
	return this->length;
}