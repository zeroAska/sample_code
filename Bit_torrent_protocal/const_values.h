/*
file:	const_values.h
proj:	Ve489 final project
by:		Peng Yuan
on:		July 6, 2015
*/

#ifndef __CONST_VALUES_H__
#define __CONST_VALUES_H__

#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <list>


extern int DEFAULT_CLIENT_LISTEN_PORT;
#define NAT_WAIT_TIME 1
const char ALL_FILE_STATUS_PATH[] = "torrents.status";
#define SHA1_LENGTH 20
#define PACKET_DATA_SIZE 2000
#define PIECE_DATA_IN_PCK_SIZE PACKET_DATA_SIZE-8
#define GENERAL_DATA_IN_PCK_SIZE PACKET_DATA_SIZE-4
#define FILE_NAME_LIMIT 50
#define TORRENT_BUFFER_SIZE 1946
#define PACKET_CTR_SIZE 1
#define PACKET_SIZE PACKET_DATA_SIZE+PACKET_CTR_SIZE
// packet fix size PACKET_DATA_SIZE + PACKET_CTR_SIZE byte, type:
const char PKT_TYPE_UP_TORRENT = 'T'; //for uploading torrent;
const char PKT_TYPE_TR_SHA1 = 'C'; //for sending check sum of torrent;
const char PKT_TYPE_CON_INFO = 'I'; //for connection info type;
const char PKT_TYPE_GET_PEER_LIST = 'R'; //for requiring peer list from tracker
const char PKT_TYPE_CON_INFO_GROUP = 'i'; //for 8 connection info type together
const char PKT_TYPE_PIECE_DATA = 'P'; //for piece data to peers
const char PKT_TYPE_END = 'X'; //for transsmision finished
const char PKT_TYPE_GET_STATUS = 'r'; //for require pieces status list
const char PKT_TYPE_STATUS = 'S'; //for status list
const char PKT_TYPE_ERROR = 'E'; //for error
const char PKT_TYPE_GET_PIECE = 'D'; //for download certain piece requiring to a peer
const char PKT_TYPE_GET_TORRENT_LIST = 'L'; // for download torrent list from server
const char PKT_TYPE_TORRENT_LIST = 'l'; // return from server about torrent list
const char PKT_TYPE_GET_TORRENT = 'K'; // for download a certain torrent from server
const char PKT_TYPE_TORRENT = 'k'; // reture from server with torrent data
const char PKT_TYPE_ACK_TRACKER = 'A'; // used to keep the NAT open
const char PKT_TYPE_TR_SHA1_MAC = 'M'; //for sending check sum of torrent;

//#endif


/*#ifndef __CLOBEL_STRUCTS__
#define __CLOBEL_STRUCTS__

#ifndef PACKET_CTR_SIZE
#define PACKET_CTR_SIZE 1
#endif

#ifndef PACKET_DATA_SIZE
#define PACKET_DATA_SIZE 1
#endif
*/

#define SIZE_OF_IP 20
struct connection_info_t   // the wrapper of ip and port
{
	char ip[SIZE_OF_IP];
	uint32_t port_num;
};

#define SIZE_OF_SHA1_MAC_T SIZE_OF_IP+24
struct sha1_and_mac_t
{
	uint32_t sha1[5];
	connection_info_t my_ip_port;
};

#define SIZE_OF_CONNECTION_INFO 8
struct connection_info_t_i
{
	uint32_t used_slots_num;
	connection_info_t ip_port_zip[SIZE_OF_CONNECTION_INFO];
};

struct peer_t               // The status of an ip.
{
	bool isConnected;
	connection_info_t conInfo;
	char * pieces_status;
};

struct piece_t              // a piece in the file
{
	uint32_t sha1[5];
	unsigned int piece_num;
	unsigned int length;
	unsigned long offset;       // offset in the file
};

struct torrent_up_t
{
	uint32_t tr_data_size;
	char tr_file_name[FILE_NAME_LIMIT];
	char tr_data[TORRENT_BUFFER_SIZE];
};

struct general_transfer_data_t
{
	uint32_t data_size;
	char pce_data[GENERAL_DATA_IN_PCK_SIZE];
};

struct piece_data_t
{
	uint32_t the_piece_num;
	uint32_t the_data_length;
	char pce_data[PIECE_DATA_IN_PCK_SIZE];
};

struct file_compressed_t
{
	std::string file_path, torrent_path;
	const char * strored_pieces_info;
};

struct down_from_peer_msg_t
{
	unsigned int position_in_vector;
	pthread_mutex_t * mtx;
	pthread_mutex_t * status_mtx;
	pthread_cond_t * cond;
	unsigned int position_in_peer_vec;
	char * this_peer_status_list;
	class socket_c * socket_to_this_peer;
	char * thread_indicator;
	class file_c * the_file_obj;
};

#endif

