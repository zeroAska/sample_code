
#ifndef __TRACKER_C_H__
#define __TRACKER_C_H__
#include <map>
#include <string>
#include <list>
#include "const_values.h"
#include "packet_c.h"
#include "socket_c.h"
#include "common.h"
class tracker_c
{
public:
    tracker_c();
    tracker_c(std::string & peer_list_path); // read the peer list file and start the tracker 
    ~tracker_c();
	void listen(int port);
	
    int get_peer_list(packet_c &pkt, socket_c *sock);
	// 1. client ask for one peer list from tracker
	//    tracker receive one pkt: PKT_TYPE_GET_PEER_LIST, content: new struct: SHA1 + mac
	//  
	//TODO    tracker send several pkts: PKT_TYPE_CON_INFO_GROUP, content: connection_info_t_i
	//    tracker send one pkt: 'X'
	//    

	int ping_from_client(packet_c &pkt, socket_c *sock);
	// tracker receive one pkt: PKT_TYPE_ACK_TRACKER ; content: MAC char[13] with '\0'   
	// update the map
	
    int update_peer_list(packet_c &pkt, socket_c *sock); // add the hash code to the peer list in memory and on disk. Call broadcast_updated_peer_list() to tell other peers about this change.
	
	void ack_peer_of_a_new_peer(connection_info_t remote_connection, connection_info_t to_connection, uint32_t * received_sha1);

	/* 1. register new torrent 
	 *    
	 *    tracker receive one pkt: PKT_TYPE_SHA1_MAC. content: SHA + mac
	 */
	//void update_peer_list_in_mem();
//private:
	socket_c * listen_socket;
	/* In this folder, each file is the record for each torrent's peers. 
	* The format is: ip,port \n ip,port \n ...
	*
	*/
	std::list<socket_c *> out_sockets;
	std::string peer_list_folder;// = "/peer_list/";
    // TODO 
    std::map<std::string, std::list<connection_info_t *>*> peer_list_in_mem; // the map, key is the hash code of each torrent file, value is the list of peers.
	//std::list<connection_info_t> broadcast_updated_peer_list(std::string & torrent_sha1sum); // when one machine started download/upload a file, it will tell tracker. Then tracker will broadcast to all other peers.
};


#endif
