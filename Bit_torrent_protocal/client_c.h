/*
file:	client.h
proj:	Ve489 final project
by:		Peng Yuan
on:		July 12, 2015
*/

#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <fstream>
#include <unistd.h>
#include <string>
#include <cstring>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include "file_c.h"
#include "packet_c.h"
#include "socket_c.h"
#include "sha1.h"
#include "const_values.h"

struct thd_listen_to_peer_t
{
	int port_num;
	class client_c * this_obj;
};

struct ptrCmp
{
    bool operator()( const uint32_t * s1, const uint32_t * s2 ) const
    {
        return memcmp( s1, s2, SHA1_LENGTH ) < 0;
    }
};

class client_c
{
public:
	client_c();
	~client_c();
	void interface(void);
	void set_server(connection_info_t server_info);
	void download_torrent_list(void);
	void download_torrent(const std::string & torrent_name);
	void gen_torrent(const std::string & file_path, const std::string & torrent_path, connection_info_t tracker_info);
	void upto_server(const std::string & torrent_path);
	void reg_tracker(const std::string & torrent_path);
	void download_the_file(const std::string & torrent_path, const std::string & file_path);
//private:
	std::map<std::string, file_c *> trpath_file_map;
	std::map<uint32_t *, file_c *, ptrCmp> trhash_file_map;
	connection_info_t cur_server;
	std::vector<pthread_t *> to_be_freed;
	bool exit_flag;
	bool ack_tracker_flag;
	bool listen_flag;
	void listen_to_peer(int port);
	void my_exit(void);
};

#endif