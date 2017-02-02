#include "tracker_c.h"
#include <dirent.h>
#include <list>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include "const_values.h"
#include <algorithm>
#include "common.h"
using namespace std;

pthread_mutex_t mutex;

static void display_peer_list_map(map<string, list<connection_info_t *> *> & m)
{
	printf("[Server] file-peer map is\n");
	for  (auto &p : m)
	{
		string key = p.first;
		list<connection_info_t *  > * l = p.second;
		cout<<key<<"\t";
		for (auto &val : *l)
		{
			cout<<string(val->ip)<<":"<<val->port_num<<", ";
		}
		cout<<endl;
	}
}

tracker_c::tracker_c()
{
	cout<<"please use the constructor with parameters!"<<endl;
}

tracker_c::tracker_c(string & peer_file_path)
{
	cout<<"Init Tracker..."<<endl;
    peer_list_folder = peer_file_path;
    pthread_mutex_init(&mutex,NULL);
    DIR * d = opendir( peer_list_folder.c_str() );
    if (d == NULL) 
    {
    	error_print("[ls] folder: "+peer_list_folder+" doesn't exist!");
    	return;
    }
    for (dirent * de = NULL; (de = readdir(d)) != NULL;)
    {
    	string file_name(de->d_name);
    	if (file_name == string(".") || file_name == string("..")) continue;
    	string file_path = peer_list_folder + '/'+file_name;
    	ifstream infile(file_path);
		string line,ip_str,port_str;
		string::size_type pos;
		// TODO: must we new this list?
		list<connection_info_t *> *l = new list<connection_info_t *>;
		peer_list_in_mem[file_name] = l;
		while (getline(infile,line))
		{
			struct connection_info_t  * new_ip_port = new struct connection_info_t;
			pos = line.find(',');
			ip_str = line.substr(0,pos);
			port_str = line.substr(pos+1,line.size()-pos-1);
			memcpy(new_ip_port->ip,ip_str.c_str(),ip_str.size());
			//new_ip_port->port_num = (uint32_t)stoi(port_str);
			new_ip_port->port_num = (uint32_t)stoi(port_str);
			l->push_back(new_ip_port);		
		}
		infile.close();
    }
    display_peer_list_map(peer_list_in_mem);  
	cout<<"Init tracer finished."<<endl;
}

tracker_c::~tracker_c()
{
 	//map<uint32_t, list<connection_info_t *> *>::iterator map_iter;
 	for (auto & hash_and_list :peer_list_in_mem)
 	{
 		string hash_code = hash_and_list.first;
 		string file_path = peer_list_folder + (hash_code);
 		list<connection_info_t *> * l = hash_and_list.second;
 		ofstream outfile(file_path);
 		
 		for (auto && connection : *l)
 		{
 			outfile<<connection->ip<<','<<connection->port_num;
 			delete connection;
 		}

 		delete l;
 	}
 	delete listen_socket;
}   



void tracker_c::listen(int port)
{
	listen_socket = new socket_c();
	
	if (listen_socket->my_listen(port) == false)
	{
		error_print("[listen] accept failed");
		delete listen_socket;
		return;
	}
	packet_t * pkt_buf = new packet_t;	
	int indicator = 0;
	while (1)
	{
		packet_c *pkt = new packet_c;
		int client_socket = listen_socket->my_accept();
		listen_socket->receive_single_pkt(*pkt_buf);
		pkt->load_packet(pkt_buf);
		socket_c * thread_socket = new socket_c (client_socket);
		switch (pkt->get_type())
		{

   	     	case PKT_TYPE_GET_PEER_LIST: get_peer_list(*pkt,thread_socket);
				indicator = 0;
				break;

		
		    case PKT_TYPE_ACK_TRACKER: ping_from_client(*pkt,thread_socket);
				indicator = 0;
				break;

			
			case PKT_TYPE_TR_SHA1_MAC: update_peer_list(*pkt,thread_socket);
				indicator = 0;
				break;

			
			default: error_print("[listen] Unsuppported packet type");
				indicator = 1;
				break;
		}
		if (indicator == 0)
		{
			out_sockets.push_back(thread_socket);
	        //listen_socket = new socket_c;
	        // TODO: in this case, the listen port has to change all the time?
			//if (listen_socket->listen_and_accept(port) == false)
			//	error_print("socket listen and accept failed");
        }
		bzero(pkt_buf->data,PACKET_DATA_SIZE);
		bzero(pkt_buf->type,PACKET_CTR_SIZE);
	}
	delete pkt_buf;
}

// 1. client ask for one peer list from tracker
//    tracker receive one pkt: PKT_TYPE_GET_PEER_LIST, content: new struct: SHA1 + mac
//  
//TODO    tracker send several pkts: PKT_TYPE_CON_INFO_GROUP, content: connection_info_t_i
//    tracker send one pkt: 'X'
//   
static void * get_peer_list_func(void * obj)
{
	cout<<"[get_peer_list_func] start"<<endl;
	// check the map for the peer list
	struct pthread_arg_packet_socket * arg = (struct pthread_arg_packet_socket *)obj;
	packet_c *pkt = arg->packet;
	socket_c *sock = arg->sock;
	tracker_c * tracker = (tracker_c *)arg->this_ptr;
	struct sha1_and_mac_t * received = (struct sha1_and_mac_t *) pkt->get_data();
	string place_holder;
	uint32_to_string(place_holder, received->sha1,5);
	connection_info_t remote_connection = received -> my_ip_port;
	cout<<"[get_peer_list] the file is "<<place_holder<<endl;
	if (tracker->peer_list_in_mem.count(place_holder) == 0)
	{
		// no such key
		printf("ERROR no such torrent\n");
		return NULL;
	}
	// make connection_info_i_t
	list<connection_info_t *> * l = tracker->peer_list_in_mem[place_holder];
	//int count = 0;
	struct connection_info_t_i group;
	group.used_slots_num = 0;

	bool found = false;

	for (const connection_info_t * info : *l)
	{
		if ( strcmp( info->ip,remote_connection.ip ) == 0 && info->port_num == remote_connection.port_num )
		{
			found = true;
		}
	}

	for (const connection_info_t * connection : *l)
	{
		if (found == 0)
		{
			tracker -> ack_peer_of_a_new_peer(remote_connection, *connection, received -> sha1);
		}
		if (group.used_slots_num == 8)
		{
			packet_c pkt_group;
			pkt_group.set_type(PKT_TYPE_CON_INFO_GROUP);
			pkt_group.put_obj((void *)&group,sizeof(group));
			sock->my_send(&pkt_group);

			group.used_slots_num = 0;
		}
		else
		{
			group.ip_port_zip[group.used_slots_num++] = *connection;
			//count++;
		}

	}
	if (group.used_slots_num != 0)
	{
		packet_c pkt_group_last;
		pkt_group_last.set_type(PKT_TYPE_CON_INFO_GROUP);
		pkt_group_last.put_obj((void *)&group,sizeof(group));
		sock->my_send(&pkt_group_last);
	}

	packet_c eof_pkt;
	eof_pkt.set_type(PKT_TYPE_END);
	sock->my_send(&eof_pkt);

	//list<connection_info_t *> * l = tracker->peer_list_in_mem[place_holder];
	if (found == false)
	{
		connection_info_t * new_connection = new connection_info_t;
		memcpy( new_connection->ip, remote_connection.ip, sizeof(remote_connection.ip) );
		new_connection->port_num = remote_connection.port_num;
		pthread_mutex_lock(&mutex);
		tracker->peer_list_in_mem[place_holder]->push_back(new_connection);
		pthread_mutex_unlock(&mutex);
	}
    display_peer_list_map(tracker->peer_list_in_mem);  

	delete sock;
	delete arg;
	delete pkt;
	return NULL;
}

int tracker_c::get_peer_list(packet_c &pkt, socket_c *sock)
{

	//struct sha1_and_mac_t * received = (struct sha1_and_mac_t *) pkt.get_data();
	pthread_t      tid;  // thread ID
	pthread_attr_t attr; // thread attribute
 
	// set thread detachstate attribute to DETACHED 
	pthread_attr_init(&attr);	
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	struct  pthread_arg_packet_socket * pthread_arg = new pthread_arg_packet_socket;
 	pthread_arg->packet = &pkt;
	pthread_arg->sock = sock;
 	pthread_arg->this_ptr = (void *) this;

	if (pthread_create(&tid, &attr, get_peer_list_func, (void *)pthread_arg) != 0)
	{	
		error_print("error when create pthread function: get_peer_list");
		return -1;
	}
	else
		printf("Successfully created get_peer_list func thread\n");	
	return 0;

}


static void * ping_from_client_func(void * obj)
{
	// check the map for the peer list
	struct pthread_arg_packet_socket * arg = (struct pthread_arg_packet_socket *)obj;
	packet_c *pkt = arg->packet;
	socket_c *sock = arg->sock;
	tracker_c * tracker = (tracker_c *)arg->this_ptr;

	struct sha1_and_mac_t sha_mac = *((struct sha1_and_mac_t *) (pkt->get_data()) );
	string hash_code;
	uint32_to_string(hash_code, sha_mac.sha1,5);

	// check if peer in the map
	connection_info_t remote_connection;
	sock->get_client_host(remote_connection);

	// TODO: may be the client should send the file name or hash to the tracker??
	if (tracker->peer_list_in_mem.find(hash_code) == tracker->peer_list_in_mem.end())
	{
		// no such key
		tracker->peer_list_in_mem[hash_code] = new list<connection_info_t *>;
		connection_info_t * new_connection = new connection_info_t;
		memcpy( new_connection->ip, remote_connection.ip, sizeof(remote_connection.ip) );
		new_connection->port_num = remote_connection.port_num;
		tracker->peer_list_in_mem[hash_code]->push_back(new_connection);
	}
	else 
	{
		list<connection_info_t *> * l = tracker->peer_list_in_mem[hash_code];
		bool found = false;

		for (const connection_info_t * info : *l)
		{
			if ( strcmp( info->ip,remote_connection.ip ) == 0 && info->port_num == remote_connection.port_num )
			{
				found = true;
			}
		}
		if (found == false)
		{
			connection_info_t * new_connection = new connection_info_t;
			memcpy( new_connection->ip, remote_connection.ip, sizeof(remote_connection.ip) );
			new_connection->port_num = remote_connection.port_num;
			tracker->peer_list_in_mem[hash_code]->push_back(new_connection);
		}
	}
	delete sock;
	delete arg;
	delete pkt;
	return NULL;
}


// TOOD: use a function pointer 

int tracker_c::ping_from_client(packet_c &pkt,socket_c * sock)
{
	//struct sha1_and_mac_t * received = (struct sha1_and_mac_t *) pkt.get_data();
	pthread_t      tid;  // thread ID
	//pthread_attr_t attr; // thread attribute
 
	// set thread detachstate attribute to DETACHED 
	//thread_attr_init(&attr);	
	//pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	struct  pthread_arg_packet_socket * pthread_arg = new pthread_arg_packet_socket;
 	pthread_arg->packet = &pkt;
	pthread_arg->sock = sock;
 	pthread_arg->this_ptr = (void *) this;

	if (pthread_create(&tid, NULL, ping_from_client_func, (void *)pthread_arg) != 0)
	{	
		error_print("error when create pthread function: get_peer_list");
		return -1;
	}
	else
		printf("Successfully created get_peer_list func thread\n");	
	return 0;
}


static void * update_peer_list_func(void * obj)
{
	struct pthread_arg_packet_socket * arg = (struct pthread_arg_packet_socket *)obj;
	packet_c *pkt = arg->packet;
	socket_c *sock = arg->sock;
	tracker_c * tracker = (tracker_c *)arg->this_ptr;

	struct sha1_and_mac_t sha_mac = *((struct sha1_and_mac_t *) (pkt->get_data()) );
	string hash_code;
	uint32_to_string(hash_code, sha_mac.sha1,5);

	connection_info_t remote_connection = sha_mac.my_ip_port;
	//sock->get_client_host(remote_connection);
	cout<<"[Update peer list] received file hash code is "<<hash_code<<endl;
	if (tracker->peer_list_in_mem.count(hash_code) == 0)
	{
		// no such key
		pthread_mutex_lock(&mutex);
		tracker->peer_list_in_mem[hash_code] = new list<connection_info_t *>;
		connection_info_t * new_connection = new connection_info_t;
		memcpy( new_connection->ip, remote_connection.ip, sizeof(remote_connection.ip) );
		new_connection->port_num = remote_connection.port_num;
		tracker->peer_list_in_mem[hash_code]->push_back(new_connection);
		pthread_mutex_unlock(&mutex);
	}
	else 
	{
		printf("[update peer list] torrent exist in tracker\n");
		
		list<connection_info_t *> * l = tracker->peer_list_in_mem[hash_code];
		bool found = false;

		for (const connection_info_t * info : *l)
		{
			if ( strcmp( info->ip,remote_connection.ip ) == 0 && info->port_num == remote_connection.port_num )
			{
				found = true;
			}
		}
		if (found == false)
		{
			connection_info_t * new_connection = new connection_info_t;
			memcpy( new_connection->ip, remote_connection.ip, sizeof(remote_connection.ip) );
			new_connection->port_num = remote_connection.port_num;
			pthread_mutex_lock(&mutex);
			tracker->peer_list_in_mem[hash_code]->push_back(new_connection);
			pthread_mutex_unlock(&mutex);
		}
		
	}
    display_peer_list_map(tracker->peer_list_in_mem);  

	delete sock;
	delete arg;
	delete pkt;
	return NULL;
}

int tracker_c::update_peer_list(packet_c &pkt, socket_c *sock)
{
	//struct sha1_and_mac_t * received = (struct sha1_and_mac_t *) pkt.get_data();
	pthread_t      tid;  // thread ID
	pthread_attr_t attr; // thread attribute
 
	// set thread detachstate attribute to DETACHED 
	pthread_attr_init(&attr);	
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	struct  pthread_arg_packet_socket * pthread_arg = new pthread_arg_packet_socket;
 	pthread_arg->packet = &pkt;
	pthread_arg->sock = sock;
 	pthread_arg->this_ptr = (void *) this;

	if (pthread_create(&tid, &attr, update_peer_list_func, (void *)pthread_arg) != 0)
	{	
		error_print("error when create pthread function: get_peer_list");
		return -1;
	}
	else
		printf("Successfully created update_peer_list func thread\n");	
	return 0;
}

void tracker_c::ack_peer_of_a_new_peer(connection_info_t remote_connection, connection_info_t to_connection, uint32_t * received_sha1)
{
	socket_c * to_peer = new socket_c(to_connection);
	to_peer -> my_connect();
	packet_c * out_packet = new packet_c;
	out_packet -> set_type(PKT_TYPE_ACK_TRACKER);
	sha1_and_mac_t * send_buf = new sha1_and_mac_t;
	memcpy(send_buf -> sha1, received_sha1, 20);
	send_buf -> my_ip_port = remote_connection;
	out_packet -> put_obj((void *)send_buf, SIZE_OF_SHA1_MAC_T);
	to_peer -> my_send(out_packet);
	delete send_buf;
	delete out_packet;
	delete to_peer;
	return;
}
