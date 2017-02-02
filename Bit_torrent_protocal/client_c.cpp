/*
file:	client.cpp
proj:	Ve489 final project
by:		Peng Yuan
on:		July 12, 2015
*/

#include <fstream>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <cstring>
#include <cassert>
#include <vector>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>    //write
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include "file_c.h"
#include "packet_c.h"
#include "socket_c.h"
#include "sha1.h"
#include "const_values.h"
#include "client_c.h"

static void * listen_to_peer_func(void * thd_listen_to_peer_obj);
//static void * ack_tracker_func(void * thd_ack_tracker_obj);
//void get_my_ip(char * my_ip);
void mac_eth0(char MAC_str[13]);

struct thd_ack_tracker_t
{
	class file_c * the_file_ptr;
	class client_c * client_obj;
};

client_c::client_c()	// main() for a client
{
	this -> exit_flag = 0;
	this -> listen_flag = 0;
	this -> ack_tracker_flag = 0;	// used for thread keep waking the NAT to tracker
	printf("[client] loading stored status...\n");
	std::ifstream ifile(ALL_FILE_STATUS_PATH, std::ios::in|std::ios::binary);	// load stored info from file
	file_compressed_t * the_info = new file_compressed_t;
	//char * line = new char[2000];
	std::string line_str;
	std::string temp_strored_pieces_info;
	if (!ifile)
	{
		printf("status file does not exist\n");
	}
	for (int i = 0; i < 5; ++i) // while stored information file exist, restore all information
	{
		//printf("stored information found! LOOP %d\n", i);
		getline(ifile, line_str);
		//printf("checking point %s\n", line_str.c_str());
		if (line_str.length() == 0 || ifile.eof())
		{
			break;
		}
		std::stringstream my_sstream(line_str);
		char file_path_buf[30];
		char torrent_path_buf[30];
		my_sstream >> torrent_path_buf >> file_path_buf >> temp_strored_pieces_info;
		the_info -> file_path = file_path_buf;
		the_info -> torrent_path = torrent_path_buf;
		the_info -> strored_pieces_info = temp_strored_pieces_info.c_str();
		file_c * one_file = new file_c(* the_info);				// restore each file calss
		this -> trpath_file_map[the_info -> torrent_path] = one_file;
		this -> trhash_file_map[one_file -> sha1sum_torrent] = one_file;
		printf("loaded: %s\n", the_info -> torrent_path.c_str());
	}
	ifile.close();
	//delete[] line;
	delete the_info;
	printf("[client] loading finished!\n");
}

client_c::~client_c()
{
	this -> my_exit();	// strore information and free memory
}

void client_c::my_exit(void)
{
	if (this -> exit_flag == 1)
	{
		//printf("what? \n");
		return;
	}
	printf("[client] storing status...\n");
	std::ofstream ofile(ALL_FILE_STATUS_PATH, std::ios::out|std::ios::binary);

	std::map<std::string, file_c *, ptrCmp>::iterator it;
	for (it = this -> trpath_file_map.begin(); it != this -> trpath_file_map.end(); ++it)	// go through the entire map
	{
		ofile << it -> first << ' ' << it -> second -> file_path << ' ' << it -> second -> pieces_status << std::endl;
		delete it -> second;
	}
	ofile << std::endl;
	for (unsigned int i = 0; i < this -> to_be_freed.size(); ++i)
	{
		delete to_be_freed[i];
	}
	this -> exit_flag = 1;
	ofile.close();
	printf("[client] storing done!\n");
	return;
}

void client_c::interface(void)
{
	std::string cmd;
	std::string torrent_path, file_path;
	connection_info_t conInfo;
	while(true)
	{
		printf("[client] $cmd$ ");
		std::cin >> cmd;
		if (cmd == "listen") // listen 8887
		{
			std::cin >> DEFAULT_CLIENT_LISTEN_PORT;
			if (this -> listen_flag == 0)
			{
				this -> listen_to_peer(DEFAULT_CLIENT_LISTEN_PORT);
			}
		}
		else if (cmd == "setserver") // setserver 192.168.145.2 8888
		{
			std::cin >> conInfo.ip >> conInfo.port_num;
			this -> set_server(conInfo);
		}
		else if (cmd == "list") // list
		{
			this -> download_torrent_list();
		}
		else if (cmd == "downloadtr") // downloadtr File1.tr
		{
			std::cin >> torrent_path;
			this -> download_torrent(torrent_path + ".tr");
		}
		else if (cmd == "gentorrent") // gentorrent File1 192.168.0.2 8888
		{
			std::cin >> file_path >> conInfo.ip >> conInfo.port_num;
			this -> gen_torrent(file_path, file_path + ".tr", conInfo);
		}
		else if (cmd == "uptoserver") // uptoserver File1.tr
		{
			std::cin >> torrent_path;
			this -> upto_server(torrent_path + ".tr");
		}
		else if (cmd == "regtracker") // regtracker File1.tr
		{
			std::cin >> torrent_path;
			this -> reg_tracker(torrent_path + ".tr");
		}
		else if (cmd == "downloadfile") // download File1.tr File1
		{
			std::cin >> torrent_path >> file_path;
			this -> download_the_file(torrent_path, file_path);
		}
		else if (cmd == "sleep") // sleep 1(sec)
		{
			int sec;
			std::cin >> sec;
			sleep(sec);
		}
		else if (cmd == "exit") // exit
		{
			this -> my_exit();
			break;
		}
		else
		{
			printf("[client] invalid command!\n");
		}
	}
	return;
}

void client_c::set_server(connection_info_t server_info)
{
	this -> cur_server = server_info;
	return;
}

void client_c::download_torrent_list(void)
{
	socket_c * my_socket = new socket_c(this -> cur_server);	// connection to server
	if (my_socket -> my_connect() == false)
	{
		printf("[client] failed creating socket to server!\n");
		return;
	}
	packet_c * my_packet = new packet_c;
	packet_c * in_packet = new packet_c;
	packet_t * pkt_buf = new packet_t;

	//printf("[client] [Download torrent list] start\n");
	my_packet -> set_type(PKT_TYPE_GET_TORRENT_LIST); // request for torrent list
	my_socket -> my_send(my_packet);
	while (true)
	{
		my_socket -> receive_single_pkt(*pkt_buf);
		in_packet -> load_packet(pkt_buf);
		if (in_packet -> get_type() != PKT_TYPE_TORRENT_LIST)
		{
			if (in_packet -> get_type() == PKT_TYPE_ERROR)
			{
				printf("[client] [download_torrent_list] error occured, torrent list update failed\n");
			}	
			break;
		}
		char * torrent_list_str = (char *) in_packet -> get_data();
		printf("[client] [Download Torrent List] The list is\n %s", torrent_list_str);	// NOTE: TODO: server should return a string containing '\n'
	}
	printf("\n");
	printf("[client] $cmd$ ");
	delete pkt_buf;
	delete in_packet;
	delete my_packet;
	delete my_socket;
	return;
}

void client_c::download_torrent(const std::string & torrent_name)
{
	printf("[client] [download_torrent] start...");
	std::ofstream ofile(torrent_name.c_str(), std::ios::out|std::ios::binary);

	socket_c * my_socket = new socket_c(this -> cur_server);	// connect to server
	if (my_socket -> my_connect() == false)
	{
		printf("[client] failed creating socket to server!\n");
		return;
	}
	packet_c * my_packet = new packet_c;
	packet_c * in_packet = new packet_c;
	packet_t * pkt_buf = new packet_t;
	//printf("[client] [download_torrent] begin to send torrent request...");
	my_packet -> set_type(PKT_TYPE_GET_TORRENT);	// download a certain torrent
	my_packet -> put_obj((void *)torrent_name.c_str(), torrent_name.length() + 1);
	my_socket -> my_send(my_packet);

	SHA1 sha1_for_file;
	uint32_t its_sha1[5];
	sha1_for_file.Reset();

	while (true)	// download a certain torrent
	{
		my_socket -> receive_single_pkt(*pkt_buf);

		in_packet -> load_packet(pkt_buf);
		printf("TYPE ::: %c\n", in_packet -> get_type());
		if (in_packet -> get_type() != PKT_TYPE_TORRENT)
		{
			if (in_packet -> get_type() == PKT_TYPE_ERROR)
			{
				printf("[client] error occured, download torrent failed\n");
			}
			else if (in_packet -> get_type() == PKT_TYPE_TR_SHA1)
			{
				const uint32_t * temp_sha1 = (const uint32_t *) in_packet -> get_data();
				sha1_for_file.Result(its_sha1);
				if (memcmp(its_sha1, temp_sha1, SHA1_LENGTH) != 0)
				{
					printf("[client] hash for the torrent mismatch!!\n");
				}
				else
				{
					printf("[client] hash for the torrent matched\n");
				}
			}
			break;
		}
		//general_transfer_data_t * torrent_data_ptr = (general_transfer_data_t *) in_packet -> get_data();
		//ofile.write(torrent_data_ptr -> pce_data, torrent_data_ptr -> data_size);	// write to file
		const char * buf = (const char *) in_packet -> get_data();
		sha1_for_file << buf;
		ofile << buf; //TODO:
		//std::cout << buf;
	}
	printf("[client] torrent %s downloaded!\n", torrent_name.c_str());
	ofile.close();
	delete pkt_buf;
	delete in_packet;
	delete my_packet;
	delete my_socket;
	return;
}

void client_c::gen_torrent(const std::string & file_path, const std::string & torrent_path, connection_info_t tracker_info)
{
	class file_c * one_file = new file_c(file_path);
	one_file -> gen_torrent(torrent_path, tracker_info);
	this -> trpath_file_map[torrent_path] = one_file;
	this -> trhash_file_map[one_file -> sha1sum_torrent] = one_file;
	return;
}

void client_c::upto_server(const std::string & torrent_path)
{
	class file_c * one_file = this -> trpath_file_map[torrent_path];
	one_file -> upload_to_server(this -> cur_server);
	return;
}

void client_c::reg_tracker(const std::string & torrent_path)
{
	class file_c * one_file = this -> trpath_file_map[torrent_path];
/*
	if (ack_tracker_flag == 0) // then creat connection to tracker and keep NAT open
	{
		ack_tracker_flag = 1;
		pthread_t * thread = new pthread_t;
		this -> to_be_freed.push_back(thread);
		thd_ack_tracker_t * thd_ack_tracker_obj = new thd_ack_tracker_t;
		thd_ack_tracker_obj -> client_obj = this;
		thd_ack_tracker_obj -> the_file_ptr = one_file;
		pthread_create(thread, NULL, ack_tracker_func, (void *) thd_ack_tracker_obj);
	}
*/
	one_file -> update_peer_list_in_tracker(); // TODO:
	return;
}

void client_c::download_the_file(const std::string & torrent_path, const std::string & file_path)
{
	class file_c * the_file = new file_c();
	the_file -> load_torrent(file_path, torrent_path);
	this -> trpath_file_map[torrent_path] = the_file;
	this -> trhash_file_map[the_file -> sha1sum_torrent] = the_file;
/*
	if (ack_tracker_flag == 0)	// then creat connection to tracker and keep NAT open
	{
		ack_tracker_flag = 1;
		pthread_t * thread = new pthread_t;
		this -> to_be_freed.push_back(thread);
		thd_ack_tracker_t * thd_ack_tracker_obj = new thd_ack_tracker_t;
		thd_ack_tracker_obj -> client_obj = this;
		thd_ack_tracker_obj -> the_file_ptr = the_file;
		pthread_create(thread, NULL, ack_tracker_func, (void *) thd_ack_tracker_obj);
	}
*/
	the_file -> get_peer_list_from_tracker();
	the_file -> download_from_peers();
	return;
}

void client_c::listen_to_peer(int port_num)
{
	thd_listen_to_peer_t * thd_listen_to_peer_obj = new thd_listen_to_peer_t;
	thd_listen_to_peer_obj -> port_num = port_num;
	thd_listen_to_peer_obj -> this_obj = this;

	pthread_t * thread = new pthread_t;
	this -> to_be_freed.push_back(thread);
	if (0 == pthread_create(thread, NULL, listen_to_peer_func, (void *) thd_listen_to_peer_obj))	// start listening
		this -> listen_flag = 1;
	else
		printf("[client] error creating listening thread!\n");
	return;
}

static void * listen_to_peer_func(void * thd_listen_to_peer_obj)
{
	thd_listen_to_peer_t * temp = (thd_listen_to_peer_t *) thd_listen_to_peer_obj;
	class client_c * this_obj = temp -> this_obj;
	//char Pig = 'A';
	//socket_c * listening_socket = new socket_c(temp -> client_sock, Pig);
	socket_c * listening_socket = new socket_c;
	packet_c * in_packet = new packet_c;
	packet_t * pkt_buf = new packet_t;
	file_c * target_file;
	printf("[client] start listening to peers\n");
	listening_socket -> my_listen(temp -> port_num);
	while(true)
	{
		int client_socket = listening_socket -> my_accept();
		if (client_socket < 0)
		{
			printf("[client] listening socket failed\n");
			sleep(1);
			return NULL;
		}

		if (listening_socket -> receive_single_pkt(*pkt_buf) == 0)
			break;
		in_packet -> load_packet(pkt_buf);
		//
		peer_t a_new_peer;
		if (in_packet -> get_type() == PKT_TYPE_ACK_TRACKER)
		{
			sha1_and_mac_t * in_pkt_buf = (sha1_and_mac_t *) in_packet -> get_data();
			if (this_obj -> trhash_file_map.count(in_pkt_buf -> sha1) > 0)	// such file exist
			{
				printf("[client] new peer found at %s:%d\n", in_pkt_buf -> my_ip_port.ip, in_pkt_buf -> my_ip_port.port_num);
				target_file = this_obj -> trhash_file_map.at(in_pkt_buf -> sha1);
				a_new_peer.isConnected = 0;
				a_new_peer.conInfo = in_pkt_buf -> my_ip_port;
				a_new_peer.pieces_status = NULL; //new char[target_file -> pieces_num];
				//target_file -> peer_list.push_back(a_new_peer);
				pthread_mutex_lock(& mtx);
				//pthread_cond_signal(& cond);
				pthread_mutex_unlock(& mtx);
			}
		}

		if (in_packet -> get_type() == PKT_TYPE_GET_STATUS)
		/*
		expecting other client asking for a status list for a certain file
		thus find the corresponding file and give the socket to that file to handle
		*/
		{
			//printf("[client] PKT_TYPE_GET_STATUS corret!\n");
			if (this_obj -> trhash_file_map.count((uint32_t *) in_packet -> get_data()) > 0)	// such file exist
			{
				printf("[client] receive new peer connection\n");
				target_file = this_obj -> trhash_file_map.at((uint32_t *) in_packet -> get_data()); // extract from map
				socket_c * listening_socket2 = new socket_c(client_socket);
				target_file -> send_file_to_peer(* listening_socket2);
			}
			else
			{
				packet_c * out_packet = new packet_c;
				out_packet -> set_type(PKT_TYPE_ERROR);
				listening_socket -> my_send(out_packet);
				delete out_packet;
			}
		}
	}
	this_obj -> listen_flag = 0;	// error occured while listening
	delete temp;
	return NULL;
}
/*
static void * ack_tracker_func(void * thd_ack_tracker_obj)
{
	char my_ip[20];
    //char my_mac[13];
    get_my_ip(my_ip);
    //mac_eth0(my_mac);
	thd_ack_tracker_t * temp = (thd_ack_tracker_t *) thd_ack_tracker_obj;
	class client_c * const this_obj = temp -> client_obj;
	temp -> the_file_ptr -> tracker_info
	connect(my_skt, (struct sockaddr *)&sa_dst, sizeof(struct sockaddr));	// connect to tracker

}*/
/*{
	thd_ack_tracker_t * temp = (thd_ack_tracker_t *) thd_ack_tracker_obj;
	class client_c * const this_obj = temp -> client_obj;
	//int ret, my_skt, my_skt2, client_sock;

	struct sockaddr_in sa_dst, sa_loc, client;
	my_skt = socket(AF_INET, SOCK_STREAM, 0);
	memset(&sa_loc, 0, sizeof(struct sockaddr_in));
	sa_loc.sin_family = AF_INET;
    sa_loc.sin_port = htons(DEFAULT_CLIENT_LISTEN_PORT);	// set local port, which is also used in listening
    char my_ip[20];
    char my_mac[13];
    get_my_ip(my_ip);
    mac_eth0(my_mac);
    sa_loc.sin_addr.s_addr = inet_addr(my_ip); //LOCAL_IP_ADDRESS

    printf("try to bind to %s:%d\n", my_ip, DEFAULT_CLIENT_LISTEN_PORT);

    int yes = 1;
	setsockopt(my_skt, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(my_skt, (struct sockaddr *)&sa_loc, sizeof(struct sockaddr)) < 0)
    {
		printf("bind failed in ACK thread\n");
		return NULL;
	}	// bind the port to the socket
    //printf("ret\\\\\\%d\n", ret);
	
	pid_t fpid;
	fpid=fork();
	//
	//
	if (fpid < 0)   
        printf("error in fork!");   
    else if (fpid == 0)
    {
    	printf("ACK process in!!\n");
    	//sleep(1);
    	//assert(0);
    	//assert(1);
	    memset(&sa_dst, 0, sizeof(struct sockaddr_in));	
	    sa_dst.sin_family = AF_INET;
	    sa_dst.sin_port = htons(temp -> the_file_ptr -> tracker_info.port_num);	// set tracker info
	    sa_dst.sin_addr.s_addr = inet_addr(temp -> the_file_ptr -> tracker_info.ip);

	    printf("try to connect to %s:%d\n", temp -> the_file_ptr -> tracker_info.ip, temp -> the_file_ptr -> tracker_info.port_num);
		
	    connect(my_skt, (struct sockaddr *)&sa_dst, sizeof(struct sockaddr));	// connect to tracker
	    printf("ckeck point\n");
	    packet_c * my_packet = new packet_c;
		for (int i = 0; i < 5; ++i)
		{
			printf("sending pkt %d\n", i);
			my_packet -> set_type(PKT_TYPE_ACK_TRACKER);
			my_packet -> put_obj((void *) my_mac, 13);
			sleep(1);
			printf("%zd:\n", send(my_skt , my_packet -> make_packet() , PACKET_SIZE , MSG_WAITALL));
		}
		shutdown(my_skt, 2);
    	close(my_skt);
		printf("ACK process terminated!\n");
		assert(0);
    	assert(1);
	}
	//else
    //int z;         // Status return code 
    //struct sockaddr_in adr_inet;// AF_INET   
    //int len_inet;  //

    //Obtain the address of the socket: 
   
    //len_inet = sizeof adr_inet;  
    //z = getsockname(my_skt, (struct sockaddr *)&adr_inet, (socklen_t*)&len_inet);  
    //if ( z == -1 ) {  
    //   return NULL;   
    //}  
	//int local_port;
    //struct sockaddr_in sin;
	//int addrlen = sizeof(sin);
	//getsockname(my_skt, (struct sockaddr *)&sin, (socklen_t*)&addrlen);
	
	//local_port = (unsigned)ntohs(adr_inet.sin_port);
	//printf("local port at %d\n", local_port);
	sleep(10);
	//int c = sizeof(struct sockaddr_in);
	
	memset(&client, 0, sizeof(struct sockaddr_in));	
	listen(my_skt , 3);
	//sleep(1);
	printf("wating for incommings!!!!\n");
	//client_sock = accept(my_skt, (struct sockaddr *)&client, (socklen_t*)&c);
	printf("accept!!!!\n");
	
	//sa_loc.sin_family = AF_INET;
	//sa_loc.sin_port = htons(local_port);
	//sa_loc.sin_addr.s_addr = INADDR_ANY;
	//if (bind(my_skt2, (struct sockaddr *)&sa_loc, sizeof(struct sockaddr)) < 0)
    //{
	//	printf("bind failed in ACK thread\n");
//		return false;
//	}	// bind the port to the socket

    //printf("ret//////%d\n", ret);
    //socket_c cur_socket(client_skt);
   
	
	//close(my_skt);
	
	//if (this_obj -> listen_flag == 0)
	//{
	//	this_obj -> listen_to_peer(my_skt);
	//}
	//while(send(my_skt , my_packet -> make_packet() , PACKET_SIZE , MSG_WAITALL) >= 0)	// periodical conection, send ACK_TYPE packet once in <NAT_WAIT_TIME> seconds.
	//{	
//		sleep(NAT_WAIT_TIME);
//	}
	printf("ACK tracker failed!!\n");
	this_obj -> ack_tracker_flag = 0;
	delete temp;
	return NULL;
}
*/
