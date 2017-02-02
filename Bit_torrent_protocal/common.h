#include "packet_c.h"
#include "socket_c.h"
#include <string>
#include <list>

#ifndef __COMMON_H__
#define __COMMON_H__
struct pthread_arg_packet_socket
{
	packet_c * packet;
	socket_c * sock;
	void * this_ptr;
};


int error_print(std::string  error_msg);

size_t get_file_size(const char* filename);

void uint32_to_string(std::string & out_str, uint32_t * to_be_convert, size_t size);

void char_str_to_hash(char * data, size_t size, uint32_t result_array[]);

int ls(std::string & path, std::string & result);

std::string get_first_line(std::string & file_name);

int string_to_list(std::string & old_str, std::list<std::string> & l);

int file_to_map(std::string & file_path);


#endif
