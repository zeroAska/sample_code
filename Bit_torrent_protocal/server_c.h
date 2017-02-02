#include "packet_c.h"
#include "socket_c.h"
#include <string>
#include <thread>
#include <list>
#include <map>


#ifndef __PACKET_C_H_
#define __PACKET_C_H_
struct send_torrent_msg_t    // ?
{
	std::string & torrent_path;
    connection_info_t client;
};

class server_c
{
public:
    server_c();
    server_c(std::string & torrent_folder);   // starting the server from the torrent folder
    ~server_c();
    //int send_torrent_list_func(socket_c sock); // start new thread to send the torrent list to the clients
	int send_torrent_list(packet_c &pkt, socket_c * sock);
    // receive one packet. pkt: PKT_TYPE_GET_TORRENT_LIST; pkt content: empty
	// server send send packet: pkt: PKT_TYPE_TORRENT_LIST. pkt content: char *. char * end in '0'
	// server send last pkt: 'X'
	
    int send_torrent(packet_c &pkt, socket_c * sock);
	// server get one pkt: PKT_TYPE_GET_TORRENT. content: char *, with '\0'.
	// server send several pkt: PKT_TYPE_TORRENT. content: char *. last pkt '\0' in the middle of content
	// server send last pkt: 'x'
	// server send last pkt: PKT_TYPE_TR_SHA1
	
    //char * get_torrent_path(std::string & torrent_sha1sum); // get the torrent path from a hash code?
    int update_torrent_list(packet_c &pkt,socket_c * sock); //  update the torrent list when a new torrent is uploaded. Write the torrent to the torrent_path
	// server receive First packet:  pkt content: torrent_up_t; pkt: UP_TORRENT; start a new thread to send the torrent to the clients
	// server receive second last pkt: 'X'
	// server receive last pkt: 'SHA1'
	
    void listen(int port); // listen to a port
//
//private:
	std::string folder; //= "/torrents/"; // file name is just the hash of the torrent file. No .tr in the end.
	//std::string torrent_list_path; // the file is the written version of the map
    //std::map<uint32_t, std::string> torrent_list; // the torrent list in memory will be a map: the key is the hash code of torrent files, the value is the list of machine ids
	//std::map<std::string, std::string> name_to_hash;
	std::list<std::string> name_list;
	socket_c * listen_socket;
	//std::map<uint32_t,socket_c *> out_sockets;
	std::vector<class socket_c> out_sockets;

    //void write_torrent_file(packet_c torrent_packet);
};

#endif 
