/*
file:	file.cpp
proj:	Ve489 final project
by:		Peng Yuan
on:		July 5, 2015
*/

#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>    //write
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include "file_c.h"
#include "packet_c.h"
#include "socket_c.h"
#include "sha1.h"
#include "const_values.h"

#ifndef PIECE_SIZE
#define PIECE_SIZE 4000000
#endif

#ifndef FILE_THREAD_LIMIT
#define FILE_THREAD_LIMIT 100
#endif

int DEFAULT_CLIENT_LISTEN_PORT = 8887;

// used for multithreading
static void* download_thread_func(void * one_message); // open a new thread for downloading, uploading
static void* download_from_peers_func(void * argv_this_obj);
static void* upload_to_server_func(void * server_info);
static void* update_peer_list_in_tracker_func(void * my_info);
static void* send_file_to_peer_func(void * peer_socket);

file_c::file_c()	// constructor for the file to be downloaded
{
	pieces_num = 0;	// initial
}

file_c::~file_c()
{
	delete[] this -> pieces_status;	// free mem
	for (unsigned int i = 0; i < this -> to_be_freed.size(); ++i)
	{
		delete to_be_freed[i];	// mem used for threading, delete here
	}
}

file_c::file_c(const file_compressed_t & status) // restore previous status
{
	this -> load_torrent(status.file_path, status.torrent_path);	// load_torrent restore much information
	this -> pieces_status = new char[this -> pieces_num + 1];
	memcpy(this -> pieces_status, status.strored_pieces_info, this -> pieces_num);	// copy according to piecec number
	this -> pieces_status[this -> pieces_num] = '\0';
	for (unsigned int i = 0; i < this -> pieces_num; ++i)
	{
		if (this -> pieces_status[i] == '1')
		{
			this -> pieces_status[i] = '0'; // clear privious status bit for downloading
		}
	}
}

file_c::file_c(const std::string & file_path)	// constructor for the file to be cut and creat torrent
{
	this -> file_path = file_path;
	struct stat statbuff;
	if (stat(this -> file_path.c_str(), & statbuff) < 0)	// get the file size via OS
	{
		printf("[file] creating file object failed at states check\n");
		return;
	}
	this -> file_size = statbuff.st_size;	// store the file size
	this -> pieces_num = (file_size - 1) / PIECE_SIZE + 1; // get pieces number
	printf("[file] file obj created! file_size: %lu pieces_num: %u \n", this -> file_size, this -> pieces_num);
}

unsigned long file_c::get_size(void)
{
	return this -> file_size;
}

void file_c::cut(void)
{
	printf("[file] cutting!\n");
	SHA1 sha;
	unsigned long pos0, pos1, pos2;
	std::ifstream ifile(this -> file_path.c_str(), std::ios::in|std::ios::binary); // open file, readin
	char * buffer = new char[PIECE_SIZE]; 	//TODO: forget why +1 ...
	pos0 = ifile.tellg();	// beginning position
	piece_t a_piece;
	//printf("NUM to cut: %u\n", this -> pieces_num);
	for (unsigned int i = 0; i < this -> pieces_num; ++i)
	{
		sha.Reset();	// reset SHA class
		a_piece.piece_num = i;
		//this -> piece_info_list[i].piece_num = i;	// piece number
		pos1 = ifile.tellg();	// current position
		ifile.read(buffer, PIECE_SIZE);	// read piece into buffer
		ifile.clear();
		pos2 = ifile.tellg();	// new position
		//printf("pos1 is at %lu\n", pos1);
		//printf("pos2 is at %lu\n", pos2);
		a_piece.length = pos2 - pos1;
		//this -> piece_info_list[i].length = pos2 - pos1;	// get piece size
		//buffer[pos2 - pos1] = '\0';	// TODO: unnecessary?
		//printf("check 1\n");
		a_piece.offset = pos1 - pos0;
		//this -> piece_info_list[i].offset = pos1 - pos0;	// get offset
		//printf("check 2\n");
		sha.Input(buffer, pos2 - pos1);	// calc sha1 sum
		sha.Result(a_piece.sha1);	// get sha1 sum
		this -> piece_info_list.push_back(a_piece);
	}
	delete[] buffer;	// free mem
	ifile.close();		// close fstream
	this -> pieces_status = new char[this -> pieces_num + 1];
	memset(this -> pieces_status, '2', this -> pieces_num);	// indicating download finished
	this -> pieces_status[this -> pieces_num] = '\0';
	printf("[file] cutting done!\n");
	return;
}

void file_c::gen_torrent(const std::string & torrent_path, connection_info_t tracker_info)
{
	this -> cut();
	this -> torrent_path = torrent_path;
	std::ofstream ofile(torrent_path.c_str(), std::ios::out|std::ios::binary); // output
	printf("[file] start writing torrent!\n");
	ofile << file_path << std::endl;	// write to torrent
	ofile << this -> file_size << std::endl;
	ofile << pieces_num << std::endl;
	char buffer[10];
	//printf("[file] writing head info done!\n");
	for (unsigned int i = 0; i < pieces_num; ++i)
	{
		for (int j = 0; j < 5; ++j)	// write the hash for each piece
		{
			sprintf(buffer, "%X ", piece_info_list[i].sha1[j]);	
			ofile << buffer;
		}
		ofile <<  std::endl;
	}
	printf("[file] writing piece info to torrent done!\n");
	char buf[25];
	printf("[file] tracker at %s, %d\n", tracker_info.ip, tracker_info.port_num);
	sprintf(buf, "%s\n%d\n", tracker_info.ip, tracker_info.port_num);
	//ofile << this -> tracker_info.ip << std::endl;	// tracker info into the torrent
	//ofile << this -> tracker_info.port_num << std::endl;
	strcpy(this -> tracker_info.ip, tracker_info.ip);
	this -> tracker_info.port_num = tracker_info.port_num;
	ofile << buf;
	ofile.close();
	this -> calc_sha1_for_file(torrent_path, this -> sha1sum_torrent);	// calc sha1 for the torrent and store to <this -> sha1sum_torrent>
	printf("[file] gen torrent done!\n");
	return;
}

void file_c::upload_to_server(connection_info_t server_info)	// upload a new torrent to server
{
	pthread_t * threads = new pthread_t;
	this -> to_be_freed.push_back(threads);	// recorded, used for later [delete]
	thd_connection_info_t * thd_connection_info_obj= new thd_connection_info_t;
	thd_connection_info_obj -> the_conInfo = server_info;
	thd_connection_info_obj -> the_file_obj = this;	// track this file object
	pthread_create(threads, NULL, upload_to_server_func, (void *) thd_connection_info_obj); // new thread to handle the uploading
	return;
}

static void * upload_to_server_func(void * thd_connection_info_obj)
{
	thd_connection_info_t * temp = (thd_connection_info_t *) thd_connection_info_obj;
	file_c * const this_obj = temp -> the_file_obj;	// pointer const point to the corresponding file object
	connection_info_t server_info = temp -> the_conInfo;	// retreive the connection info

	//printf("[file] trying to upload %s \n", this_obj -> torrent_path.c_str());
	std::ifstream ifile(this_obj -> torrent_path.c_str(), std::ios::in|std::ios::binary);
	torrent_up_t * buffer = new torrent_up_t;	
	packet_c * my_packet = new packet_c;	// packet obj for sending
	my_packet -> set_type(PKT_TYPE_UP_TORRENT);

	printf("[file] trying to trying to upload %s to server at %s:%d\n", this_obj -> torrent_path.c_str(), server_info.ip, server_info.port_num);
	socket_c * my_socket = new socket_c(server_info);	// establish connection
	if (my_socket -> my_connect() == false)
	{
		printf("[file] failed creating socket to server!\n");
		return NULL;
	}

	strcpy(buffer -> tr_file_name, this_obj -> torrent_path.c_str());	// first set the torrent name in the packet
	unsigned int last_tellg;
	bool file_not_end_flag = 1;
	while(file_not_end_flag == 1)	// sending the torrent file, each packet contain both <torrent_path> and part of the file content
	{
		memset(buffer -> tr_data, 0, TORRENT_BUFFER_SIZE);
		last_tellg = (unsigned int)ifile.tellg();
		//printf("last tellg at %u\n", last_tellg);
		sleep(1);
		ifile.read(buffer -> tr_data, TORRENT_BUFFER_SIZE); // read torrent into buffer
		if (ifile.eof())
		{
			file_not_end_flag = 0;
		}
		ifile.clear();
		buffer -> tr_data_size = (unsigned int)ifile.tellg() - last_tellg;	// packet data size, ie, for the torrent file content
		my_packet -> put_obj((void *)buffer, PACKET_DATA_SIZE);	// make it into packet
		my_socket -> my_send(my_packet);
	}

	my_packet -> set_type(PKT_TYPE_END);	// send end
	my_packet -> put_obj((void *) this_obj -> sha1sum_torrent, SHA1_LENGTH);
	my_socket -> my_send(my_packet);

	my_packet -> set_type(PKT_TYPE_TR_SHA1);	// send torrent sha1
	my_packet -> put_obj((void *) this_obj -> sha1sum_torrent, SHA1_LENGTH);
	my_socket -> my_send(my_packet);

	printf("[file] torrent upload done\n");
	ifile.close();
	delete buffer;
	delete temp;	// parameters for this thread function
	delete my_packet;
	delete my_socket; // close socket in the destructor.
	return NULL;
}

void file_c::update_peer_list_in_tracker(void)
{
	pthread_t * threads = new pthread_t;
	this -> to_be_freed.push_back(threads);

	thd_connection_info_t * thd_connection_info_obj= new thd_connection_info_t;
	// thd_connection_info_obj -> the_conInfo = my_info		// tracker read IP and port, thus this is unnecessary
	thd_connection_info_obj -> the_file_obj = this;

	pthread_create(threads, NULL, update_peer_list_in_tracker_func, (void *) thd_connection_info_obj); // new thread
	return;
}

static void* update_peer_list_in_tracker_func(void * thd_connection_info_obj)
{
	printf("[file] registering tracker...\n");
	thd_connection_info_t * temp = (thd_connection_info_t *) thd_connection_info_obj;
	file_c * const this_obj = temp -> the_file_obj;

	socket_c * my_socket = new socket_c(this_obj -> tracker_info);	// establish connection
	if (my_socket -> my_connect() == false)
	{
		printf("[file] failed creating socket to tracker!\n");
		delete temp;
		delete my_socket;
		return NULL;
	}
	packet_c * my_packet = new packet_c;

	sha1_and_mac_t * mac_buf = new sha1_and_mac_t;
	memcpy(mac_buf -> sha1, this_obj -> sha1sum_torrent, SHA1_LENGTH);
	//mac_eth0(mac_buf -> mac);
	connection_info_t my_info_buf;
	my_info_buf.port_num = DEFAULT_CLIENT_LISTEN_PORT;
	get_my_ip(my_info_buf.ip);

	mac_buf -> my_ip_port = my_info_buf;

	my_packet -> set_type(PKT_TYPE_TR_SHA1_MAC); // request for certain peer list
	my_packet -> put_obj((void *) mac_buf, SIZE_OF_SHA1_MAC_T);
	my_socket -> my_send(my_packet);

/*
	my_packet -> set_type(PKT_TYPE_CON_INFO);
	my_packet -> put_obj((void *) & temp -> the_conInfo, sizeof(connection_info_t));
	my_socket -> my_send(my_packet);
*/
	printf("[file] registering tracker done\n");
	delete mac_buf;
	delete temp;
	delete my_packet;
	delete my_socket;
	return NULL;
}

void file_c::send_file_to_peer(socket_c & argv_peer_socket)	// continuesly sending file content to a certain peer
{
	pthread_t * thread = new pthread_t;
	this -> to_be_freed.push_back(thread);
	thd_peer_socket_t * thd_peer_socket_obj = new thd_peer_socket_t;
	thd_peer_socket_obj -> peer_socket = & argv_peer_socket;	// corresponding socket to another client
	thd_peer_socket_obj -> the_file_obj = this;
	pthread_create(thread, NULL, send_file_to_peer_func, (void *) thd_peer_socket_obj);
	return;
}

static void* send_file_to_peer_func(void * thd_peer_socket_obj)
{
	thd_peer_socket_t * temp = (thd_peer_socket_t *) thd_peer_socket_obj;
	file_c * const this_obj = temp -> the_file_obj;
	socket_c * the_socket = temp -> peer_socket;

	packet_c * out_packet = new packet_c;	// buffer for sending
	packet_c * in_packet = new packet_c;	// buffer for receiving
	out_packet -> set_type(PKT_TYPE_STATUS);
	out_packet -> put_obj((void *) this_obj -> pieces_status, this_obj -> pieces_num);
	/*	client handle the [require for status list]
	and then pass the socket to a file class object,
	thus the first operation in file class
	is to reply this and send the piece status list
	*/
	printf("[file] [sending to peer] start to send %s\n", this_obj -> file_path.c_str());
	the_socket -> my_send(out_packet);

	packet_t * in_pkt = new packet_t;
	unsigned int pieces_req_num;	// used to store which piece that the other client is asking for
	packet_c * my_packet = new packet_c;
	unsigned long pos1, pos2, pos_in_pkt_data;
	std::ifstream ifile(this_obj -> file_path.c_str(), std::ios::in|std::ios::binary); // open file
	piece_data_t * buffer = new piece_data_t;	// used when sending the content of the file
	const piece_data_t * one_piece_req; // used to know which piece the other client is asking for, (it send a piece_data_t with piece number and empty content)

	do
	{
		if ( 0 == the_socket -> receive_single_pkt(*in_pkt))
			break;	// supposed to be the other client asking for a piece
		in_packet -> load_packet(in_pkt);
		//printf("in sending theard, pkt reveived:: %c\n", in_packet -> get_type());
		if (in_packet -> get_type() == PKT_TYPE_GET_PIECE)
		{
			one_piece_req = (const piece_data_t *) in_packet -> get_data();
			pieces_req_num = one_piece_req -> the_piece_num;	// store which piece the other client needs
		}
		else if (in_packet -> get_type() == PKT_TYPE_GET_STATUS)
		{
			out_packet -> set_type(PKT_TYPE_STATUS);
			out_packet -> put_obj((void *) this_obj -> pieces_status, this_obj -> pieces_num);
			the_socket -> my_send(out_packet);
			continue;
		}
		else
		{
			//printf("[file] wrong type or disconnected\n");
			break;	// error occured or disconnected or sending finished
		}
		
		pos1 = this_obj -> piece_info_list[pieces_req_num].offset; // calc the beginning of certain piece
		if (pieces_req_num < this_obj -> pieces_num - 1)	// if it is ending of the file
		{
			pos2 = this_obj -> piece_info_list[pieces_req_num + 1].offset; 
		}
		else
		{
			pos2 = this_obj -> file_size;
		}

		ifile.seekg(pos1, std::ios::beg);	// offset readin position to the beginning of certain piece
		SHA1 send_sha;
		uint32_t send_sha_buf[5];
		send_sha.Reset();
		do // send one piece
		{
			if (pos1 + PIECE_DATA_IN_PCK_SIZE > pos2)
			{
				pos_in_pkt_data = pos2;	// store where we will reach at within a piece
			}
			else	// if it is ending of a piece
			{
				pos_in_pkt_data = pos1 + PIECE_DATA_IN_PCK_SIZE;
			}
			if (pos_in_pkt_data - pos1 > 40000000)
			{
				//printf("pos_in_pkt_data::%lu pos1::%lu pos2::%lu\n", pos_in_pkt_data, pos1, pos2);
			}
			ifile.read(buffer->pce_data, pos_in_pkt_data - pos1);	// read piece into buffer
			send_sha.Input(buffer->pce_data, pos_in_pkt_data - pos1);

			buffer -> the_piece_num = pieces_req_num;
			buffer -> the_data_length = pos_in_pkt_data - pos1;	// how much data in this packet
			my_packet -> set_type(PKT_TYPE_PIECE_DATA);
			my_packet -> put_obj((void *)buffer, PACKET_DATA_SIZE);
			the_socket -> my_send(my_packet);	// send the packet

			pos1 = ifile.tellg();	// update where we currently are within a piece 
		} while (pos1 < pos2);
		my_packet -> set_type(PKT_TYPE_END);
		the_socket -> my_send(my_packet);
		send_sha.Result(send_sha_buf);
		//printf("seeding sha[0] checked as %X\n", send_sha_buf[0]);
	} while (true);

	printf("[file] [sending to peer] done\n");
	ifile.close();		// close fstream
	delete my_packet;
	delete in_packet;
	delete out_packet;
	delete in_pkt;
	delete temp -> peer_socket;	// created in [client] class and passed in file class as a paramater
	delete temp;	// temp data for threading
	delete buffer;	// free memory
	return NULL;
}

void file_c::get_peer_list_from_tracker(void)
{
	printf("[file] getting peer info from tracker at %s:%d\n", this -> tracker_info.ip, this -> tracker_info.port_num);
	socket_c * my_socket = new socket_c(this -> tracker_info);
	if (my_socket -> my_connect() == false)
	{
		printf("[file] failed creating socket to tracker!\n");
		return;
	}
	packet_c * my_packet = new packet_c;
	packet_c * in_packet = new packet_c;
	packet_t * pkt_buf = new packet_t;
	peer_t one_peer;

	sha1_and_mac_t * mac_buf = new sha1_and_mac_t;
	memcpy(mac_buf -> sha1, this -> sha1sum_torrent, SHA1_LENGTH);
	//mac_eth0(mac_buf -> mac);
	connection_info_t my_info_buf;
	my_info_buf.port_num = DEFAULT_CLIENT_LISTEN_PORT;
	get_my_ip(my_info_buf.ip);

	mac_buf -> my_ip_port = my_info_buf;

	my_packet -> set_type(PKT_TYPE_GET_PEER_LIST); // request for certain peer list
	my_packet -> put_obj((void *) mac_buf, SIZE_OF_SHA1_MAC_T);
	my_socket -> my_send(my_packet);

	delete mac_buf;

	while (true)
	{
		my_socket -> receive_single_pkt(*pkt_buf);	// receive packet
		in_packet -> load_packet(pkt_buf);
		if (in_packet -> get_type() != PKT_TYPE_CON_INFO_GROUP) // expecting 8 connection info type
		{
			if (in_packet -> get_type() == PKT_TYPE_ERROR)
			{
				printf("[file] error occured, peer list update failed\n");
			}	
			break;
		}
		const connection_info_t_i * data_ptr = (const connection_info_t_i *) in_packet -> get_data();
		for (unsigned int i = 0; i < data_ptr -> used_slots_num; ++i)	// for each slot used in the 8 connection info type
		{
			one_peer.isConnected = false;
			one_peer.conInfo = data_ptr -> ip_port_zip[i];
			one_peer.pieces_status = NULL;
			this -> peer_list.push_back(one_peer);
		}
	}

	printf("[file] getting peer info done\n");
	delete pkt_buf;
	delete in_packet;
	delete my_packet;
	delete my_socket;
	return;
}

void file_c::load_torrent(const std::string & file_path, const std::string & torrent_path)
{
	printf("[file] start loading torrent from %s\n", torrent_path.c_str());
	this -> torrent_path = torrent_path;
	this -> file_path = file_path;
	this -> calc_sha1_for_file(torrent_path, this -> sha1sum_torrent);	// calc sha1 for torrent

	std::ifstream ifile(this -> torrent_path.c_str(), std::ios::in);
	std::ifstream ifile_test;
	ifile_test.open(this -> file_path.c_str(), std::ios::in);	// test whether the file exist, used to determine whether new download or continue downloading
	
	std::string name;
	ifile >> name; // get file name line
	//printf("ifile >> name : %s\n", name.c_str());
	unsigned long f_size, temp;
	ifile >> f_size;	// get file size line
	this -> file_size = f_size;
	//printf("ifile >> f_size : %lu\n", f_size);
	this -> pieces_num = (f_size - 1) / PIECE_SIZE + 1;
	if (!ifile_test)	// file not exist indicating new download
	{
		std::ofstream ofile(this -> file_path.c_str(), std::ios::out|std::ios::binary);
		printf("[file] this is a new file to be downloaded\n");
		ofile.seekp(f_size - 1, std::ios::beg);
		ofile.put((char)0x1A);	// create large file
		this -> pieces_status = new char[this -> pieces_num + 1];
		memset(this -> pieces_status, '0', this -> pieces_num);	// no piece downloaded
		this -> pieces_status[this -> pieces_num] = '\0';
		//printf("new file have piece_num = %d, status = %s\n", this -> pieces_num, this -> pieces_status);
		ofile.close();
	}
	
	ifile_test.close();
	printf("[file] file created/configured done\n");
	ifile >> this -> pieces_num; // get pieces_num
	//printf("pieces_num = %d\n", pieces_num);
	//sleep(5);
	std::string sha1_sum_for_piece;	// readin sha1 as srting, used later
	piece_t one_piece_info;

	temp = (pieces_num - 1) * PIECE_SIZE;
	getline(ifile, sha1_sum_for_piece); // absort \n
	for (unsigned int i = 0; i < this -> pieces_num; ++i)
	{
		getline(ifile, sha1_sum_for_piece);
		sscanf(sha1_sum_for_piece.c_str(), "%X %X %X %X %X ", // convert string into sha1 struct
			& one_piece_info.sha1[0],
			& one_piece_info.sha1[1],
			& one_piece_info.sha1[2],
			& one_piece_info.sha1[3],
			& one_piece_info.sha1[4]);
		one_piece_info.piece_num = i;
		if (i < this -> pieces_num - 1)
		{
			one_piece_info.length = PIECE_SIZE;
		}
		else
		{
			one_piece_info.length = f_size - temp;	// the last piece
		}
		one_piece_info.offset = PIECE_SIZE * i;
		//printf("loading piece_info %d done, with sha1[0] = %X\n", i, one_piece_info.sha1[0]);
		this -> piece_info_list.push_back(one_piece_info);
	}
	char ip_buf[20];
	int port_buf;
	ifile >> ip_buf >> port_buf;	// load tracker information
	strcpy(this -> tracker_info.ip, ip_buf);
	this -> tracker_info.port_num = port_buf;
	printf("[file] loaded torrent done with tracker at %s, %d\n", this -> tracker_info.ip, this -> tracker_info.port_num);
	//printf("load torrent done!\n");
	ifile.close();
	return;
}

void file_c::download_from_peers(void)
{
	pthread_t * down_thread = new pthread_t;
	this -> to_be_freed.push_back(down_thread);
	pthread_create(down_thread, NULL, download_from_peers_func, (void *)this);
	return;
}

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;	// mutex for multi thread, passing thread status, eg. ending
pthread_mutex_t status_mtx = PTHREAD_MUTEX_INITIALIZER;	// mutex for updating status list
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;	// mutex for wake up, used in max connection control
pthread_mutex_t file_mtx = PTHREAD_MUTEX_INITIALIZER;

static void* download_from_peers_func(void * argv_this_obj)
{
	printf("[file] [downloading] start\n");
	file_c * const this_obj = (file_c * const) argv_this_obj;

	char * thread_indicator = new char[FILE_THREAD_LIMIT];	// indicate for thread ending
	char * new_available_in_vec = new char[FILE_THREAD_LIMIT];	// indicate a new thread could be created
	memset(thread_indicator, 0, FILE_THREAD_LIMIT); // 0 for not exit, 1 for finished thread
	memset(new_available_in_vec, 1, FILE_THREAD_LIMIT); // 1 for available, 0 for not

	std::vector<pthread_t *> local_to_be_freed;	// local vector storing to be freed thread type
	std::vector<down_from_peer_msg_t *> msg_t_to_be_freed; // massage passed into new threads
	unsigned int count = 0;
	unsigned int one_available = 0;	// index for one avaliable theard slot
	do
	{
		for (unsigned int i = 0; i < this_obj -> peer_list.size(); ++i)	// go through entire peer list
		{
			if (count >= FILE_THREAD_LIMIT) // reach limit
			{
				break;
			}
			if (this_obj -> peer_list[i].isConnected == false)
			{
				pthread_t * one_thread = new pthread_t;
				socket_c * one_socket = new socket_c(this_obj -> peer_list[i].conInfo);	// new connection to a peer
				printf("[file] [downloading] download from peer %s:%d\n", this_obj -> peer_list[i].conInfo.ip, this_obj -> peer_list[i].conInfo.port_num);
				if (one_socket -> my_connect() == true )
				{
					// find available slot
					for (unsigned int j = 0; j < FILE_THREAD_LIMIT; ++j)
					{
						if (new_available_in_vec[j])
						{
							one_available = j;	// get next avaliable thread slot
						}
					}
					// insert information to msg
					down_from_peer_msg_t * one_message = new down_from_peer_msg_t;	// used to create new thread
					if (msg_t_to_be_freed.size() <= one_available)	// if not full
					{
						msg_t_to_be_freed.push_back(one_message);
						local_to_be_freed.push_back(one_thread);
						one_message -> position_in_vector = msg_t_to_be_freed.size() - 1;
					}
					else	// if vector full, ie reach the max allowed thread num
					{
						msg_t_to_be_freed[one_available] = one_message;
						local_to_be_freed[one_available] = one_thread;
						one_message -> position_in_vector = one_available;
					}
					one_message -> mtx = & mtx;
					one_message -> status_mtx = & status_mtx;
					one_message -> cond = & cond;
					one_message -> position_in_peer_vec = i; // require peer list don't update info of connected peers
					one_message -> socket_to_this_peer = one_socket;
					one_message -> thread_indicator = thread_indicator;
					one_message -> this_peer_status_list = new char[this_obj -> pieces_num];
					one_message -> the_file_obj = this_obj;
					this_obj -> peer_list[i].pieces_status = one_message -> this_peer_status_list; // status list for this peer
					// need to free: socket, status when thread finishes, and set NULL, not connected
					this_obj -> peer_list[i].isConnected = true;
					pthread_create(one_thread, NULL, download_thread_func, (void *) one_message); // new thread 
					count ++;
				}
				else // connection fail
				{
					delete one_thread;
					delete one_socket;
				}
			}
		}
		//sleep(5);
		pthread_mutex_lock(&mtx);
		//pthread_cond_wait(&cond, &mtx);	// wake up by signal(&cond)
		for (unsigned int i = 0; i < FILE_THREAD_LIMIT; ++i)
		{
			if (thread_indicator[i] == 1)	// check which thread ends
			{
				thread_indicator[i] = 0;
				delete msg_t_to_be_freed[i] -> socket_to_this_peer;
				delete msg_t_to_be_freed[i] -> this_peer_status_list;
				//this_obj -> peer_list[msg_t_to_be_freed[i] -> position_in_peer_vec].isConnected = false;
				this_obj -> peer_list[msg_t_to_be_freed[i] -> position_in_peer_vec].pieces_status = NULL;
				delete msg_t_to_be_freed[i];
				delete local_to_be_freed[i];
				msg_t_to_be_freed[i] = NULL;
				local_to_be_freed[i] = NULL;
				new_available_in_vec[i] = 1;
				count --;
			}
		}
		pthread_mutex_unlock(&mtx);

	} while(count > 0); // TODO: || this_obj -> check_integrity_file() == false); TODO: can we get file sha1 from torrent?
	printf("[file] [downloading] finished\n");
	delete[] thread_indicator;
	delete[] new_available_in_vec;
	return NULL;
}

static void* download_thread_func(void * one_message)
{
	down_from_peer_msg_t * the_message = (down_from_peer_msg_t *) one_message;
	file_c * const this_obj = the_message -> the_file_obj;
	socket_c * my_socket = the_message -> socket_to_this_peer;

	packet_c * my_packet = new packet_c;
	packet_c * in_packet = new packet_c;
	packet_t * pkt_buf = new packet_t;
	piece_data_t * buffer = new piece_data_t;
	const piece_data_t * inbuffer;
	std::ofstream ofile(this_obj -> file_path.c_str(), std::ios::out|std::ios::binary);

	my_packet -> set_type(PKT_TYPE_GET_STATUS); // request for certain pieces status in that peer
	// NOTE: use torrent sha1 to find corresponding file (at client)
	my_packet -> put_obj((void *) this_obj -> sha1sum_torrent, SHA1_LENGTH);
	my_socket -> my_send(my_packet);
	my_socket -> receive_single_pkt(*pkt_buf);
	in_packet -> load_packet(pkt_buf);
	//printf("in downloading theard, pkt reveived:: %c\n", in_packet -> get_type());
	if (in_packet -> get_type() != PKT_TYPE_STATUS) // expecting status list type
	{
		if (in_packet -> get_type() == PKT_TYPE_ERROR)
		{
			printf("[file] [downloading] error occured at getting status list\n");
		}
		//printf("type not match ???\n");

		delete buffer;	// free memory
		delete pkt_buf;
		delete in_packet;
		delete my_packet;
		return NULL;
	}
	memcpy(the_message -> this_peer_status_list, in_packet -> get_data(), this_obj -> pieces_num); // updata this peer status list
	SHA1 new_calc_sha1;

	unsigned long download_size = 0;
	bool same_flag = 0;
	do
	{
		unsigned int i = 0;
		unsigned int cur_piece_num = 0;
		do
		{
			cur_piece_num = 0;

			printf("this_obj -> pieces_num = %d\n", this_obj -> pieces_num);
			printf("with remote client has %s\n", the_message -> this_peer_status_list);
			printf("with my client has %s\n", this_obj -> pieces_status);
			for (i = 0; i < this_obj -> pieces_num; ++i)
			{
				if (this_obj -> pieces_status[i] == '0' && the_message -> this_peer_status_list[i] == '2')
				{
					pthread_mutex_lock(the_message -> status_mtx); // lock and update status list to '1', ie, downloading
					cur_piece_num = i;
					this_obj -> pieces_status[i] = '1';
					pthread_mutex_unlock(the_message -> status_mtx);
					break;
				}
			}

			if (i == this_obj -> pieces_num)	// no piece can be downloaded
			{
				//printf("breaked ???\n");
				break;
			}
			buffer -> the_piece_num = cur_piece_num;
			my_packet -> set_type(PKT_TYPE_GET_PIECE);
			my_packet -> put_obj((void *)buffer, PACKET_DATA_SIZE);
			the_message -> socket_to_this_peer -> my_send(my_packet);
			//printf("getting thread send type :: %c at 111\n", my_packet -> get_type());
			//printf("cur_piece_num = %u\n", cur_piece_num);
			//printf("length of piece_info_list = %zu \n", this_obj -> piece_info_list.size());
			//printf("going to seek to %lu\n", this_obj -> piece_info_list[cur_piece_num].offset);
			ofile.seekp(this_obj -> piece_info_list[cur_piece_num].offset, std::ios::beg); // write initial position
			
			new_calc_sha1.Reset();
			int cnt = 0;
			char temp[2000];
			do 	// download one piece
			{
				cnt++;
				the_message -> socket_to_this_peer -> receive_single_pkt(*pkt_buf);
				in_packet -> load_packet(pkt_buf);

				//printf("in downloading theard, pkt reveived 22:: %c\n", in_packet -> get_type());
				if (in_packet -> get_type() == PKT_TYPE_PIECE_DATA)
				{
					inbuffer = (const piece_data_t *) in_packet -> get_data();
					if (inbuffer -> the_piece_num == cur_piece_num)
					{
						pthread_mutex_lock(&file_mtx);
						ofile.write(inbuffer -> pce_data, inbuffer -> the_data_length);
						pthread_mutex_unlock(&file_mtx);
						download_size += inbuffer -> the_data_length;
						new_calc_sha1.Input(inbuffer -> pce_data, inbuffer -> the_data_length);
					}
				}
				memset(temp, 0, 2000);
				memcpy(temp, inbuffer -> pce_data, 1999);
				if (cnt == 500)
				{
					//printf("@QEDSSS ::: %s\n", temp);
					cnt = 0;
				}
			} while (in_packet -> get_type() == PKT_TYPE_PIECE_DATA); // in_packet.get_type != <END_TYPE>

			uint32_t sha_buf[5];
			new_calc_sha1.Result(sha_buf);
			if (this_obj -> check_integrity_piece(cur_piece_num, sha_buf) == true)	// check piece integrity
			{
				pthread_mutex_lock(the_message -> status_mtx);
				this_obj -> pieces_status[cur_piece_num] = '2';
				pthread_mutex_unlock(the_message -> status_mtx);
			}
			else
			{
				//printf("check piece integrity failed... at %d\n", cur_piece_num);
				sleep(2);
				pthread_mutex_lock(the_message -> status_mtx);
				this_obj -> pieces_status[cur_piece_num] = '0';
				pthread_mutex_unlock(the_message -> status_mtx);
			}
		} while(true);

		// till now, no more piece can be downloaded according to this peer's status list
		// now update this peer's status list

		my_packet -> set_type(PKT_TYPE_GET_STATUS); // request for pieces status in that peer
		// NOTE: use torrent sha1 to find corresponding file (at client)
		my_packet -> put_obj((void *) this_obj -> sha1sum_torrent, SHA1_LENGTH);
		my_socket -> my_send(my_packet);
		my_socket -> receive_single_pkt(*pkt_buf);
		in_packet -> load_packet(pkt_buf);
		if (in_packet -> get_type() != PKT_TYPE_STATUS) // expecting type stutas list
		{
			if (in_packet -> get_type() == PKT_TYPE_ERROR)
			{
				printf("[file] [downloading] error occured at getting status list\n");
			}
	
			delete buffer;	// free memory
			delete pkt_buf;
			delete in_packet;
			delete my_packet;
			return NULL;
		}
		char * temp_holder = new char[this_obj -> pieces_num];
		memcpy(temp_holder, in_packet -> get_data(), this_obj -> pieces_num);
		same_flag = (0 == memcmp(temp_holder, the_message -> this_peer_status_list, this_obj -> pieces_num));	// check if this peer's status list is updated
		memcpy(the_message -> this_peer_status_list, temp_holder, this_obj -> pieces_num);
		delete[] temp_holder;
	} while(!same_flag);
	ofile.close();
	pthread_mutex_lock(the_message -> mtx);
	the_message -> thread_indicator[the_message -> position_in_vector] = 1;	// indicate that this thread is ending
	//pthread_cond_signal(the_message -> cond);	// wake up the waiting thread
	pthread_mutex_unlock(the_message -> mtx);

	connection_info_t remote_info;
	my_socket -> get_dest_host(remote_info);
	printf("[file] download %lu Bytes from peer %s:%d\n", download_size, remote_info.ip, remote_info.port_num);
	delete buffer;	// free memory
	delete pkt_buf;
	delete in_packet;
	delete my_packet;
	return NULL;
}

bool file_c::check_integrity_piece(unsigned int the_piece_num, const uint32_t * new_calc_sha1)
{
	if (memcmp(this -> piece_info_list[the_piece_num].sha1, new_calc_sha1, 20) == 0)
	{
		return true;
	}
	//printf("with new_calc_sha1[0] = %X and original sha[0] = %X\n", new_calc_sha1[0], this -> piece_info_list[the_piece_num].sha1[0]);
	return false;
}

void file_c::calc_sha1_for_file(const std::string & the_file_path, uint32_t * its_sha1)
{
	SHA1 sha1_for_file;
	sha1_for_file.Reset();
	std::ifstream ifile(the_file_path.c_str(), std::ios::in|std::ios::binary);
	char file_char;
	while (ifile.get(file_char))
		sha1_for_file.Input(file_char);	// put char one by one till the end of the file
	sha1_for_file.Result(its_sha1);
	return;
}

bool file_c::check_integrity_file(void)
{
	uint32_t its_sha1[5];
	this -> calc_sha1_for_file(this -> file_path, its_sha1);
	// TODO: should the file sha1_sum be also write into the torrent?
	return true; // currently always return true
	//
	if (memcmp(this -> sha1sum_file, its_sha1, 20) == 0)
	{
		return true;
	}
	return false;
}
