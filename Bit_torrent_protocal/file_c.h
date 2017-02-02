/*
file:	file.h
proj:	Ve489 final project
by:		Peng Yuan
on:		July 5, 2015
*/

#ifndef __FILE_C_H__
#define __FILE_C_H__

#include <string>
#include <list>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "packet_c.h"
#include "socket_c.h"
#include "sha1.h"
#include "const_values.h"

extern int DEFAULT_CLIENT_LISTEN_PORT;
extern pthread_mutex_t mtx;	// mutex for multi thread, passing thread status, eg. ending
extern pthread_mutex_t status_mtx;	// mutex for updating status list
extern pthread_cond_t cond;

struct thd_peer_socket_t
{
	class socket_c * peer_socket;
	class file_c * the_file_obj;
};

struct thd_connection_info_t
{
	connection_info_t the_conInfo;
	class file_c * the_file_obj;
};

class file_c                 
{ 
public:
	file_c();												// constructor for the file to be downloaded
	~file_c();
	//restore
	file_c(const file_compressed_t & status); 				// restrore from status information
	//store
	//void compress_file_status_to(file_compressed_t & status); // store status information, now is in client
	// up
	file_c(const std::string & file_path);        			// constructor for the file to be cut and creat torrent
	unsigned long get_size(void);    						// get the file size
	void gen_torrent(const std::string & torrent_path, connection_info_t tracker_info); // create the torrent, according to the torrent path, the tracker’s ip, assuming that the file is well initialized
	void upload_to_server(connection_info_t server_info); 	// upload the torrent of the file to the server
	void update_peer_list_in_tracker(void); 				// add this machine’s ip, port, to the tracker.
	// up to peer
	void send_file_to_peer(socket_c & peer_socket);
	// down
	void load_torrent(const std::string & file_path, const std::string & torrent_path); // load existing torrent from existing file.
	void get_peer_list_from_tracker(void);					
	void download_from_peers(void);
	// general
	void calc_sha1_for_file(const std::string & the_file_path, uint32_t * its_sha1);
	bool check_integrity_file(void);
//private:
	std::string file_path;
	std::string torrent_path;
	unsigned int pieces_num;
	//std::string sha1sum_file;
	uint32_t sha1sum_torrent[5];
	uint32_t sha1sum_file[5];
	unsigned long file_size;
	std::vector<peer_t> peer_list;     
	std::vector<piece_t> piece_info_list;
	std::vector<pthread_t *> to_be_freed;
	char * pieces_status;       // the char array, indicating each pieces’ status, like being downloading, not exist, or finished. Like an bitmap.
	connection_info_t tracker_info;
	void cut(void);  // get a vector of pieces
	//void* download_thread_func(void * one_message); // open a new thread for downloading, uploading
	//void* download_from_peers_func(void);
	//void* upload_to_server_func(void * server_info);
	//void* update_peer_list_in_tracker_func(void * my_info);
	//void* send_file_to_peer_func(void * peer_socket);
	bool check_integrity_piece(unsigned int the_piece_num, const uint32_t * new_calc_sha1);
};

#endif