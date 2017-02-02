#include "server_c.h"
#include <unistd.h>
#include <dirent.h>
#include <list>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <pthread.h>
#include "const_values.h"
#include "common.h"
using namespace std;

static void calc_sha1_for_file(const std::string & the_file_path, uint32_t * its_sha1);

void display_map(map<string,string> & m)
{
	for (auto & pair: m)
	{
		//cout<<pair<<endl;
		string s1 = pair.first;
		string s2 = pair.second;
		cout<<s1<<",   "<<s2<<endl;
	}
}

void make_char_pkt_and_send(socket_c * sock, const char the_str[], const size_t size, char type)
{
	//printf("READ !!! hehehe %s\n", the_str);
	packet_c pkt;
	cout<<"[Server] pkt data size is "<<size<<endl;
	int num_pkt = int(size / (PACKET_DATA_SIZE - 1)) + 1;
	char data[PACKET_DATA_SIZE];
	cout<<"[Server] The number of pkt are "<<num_pkt<<endl;
	for (int t = 0; t != num_pkt; t++)
	{
    	bzero(data,PACKET_DATA_SIZE);
		memcpy(data,the_str + t * (PACKET_DATA_SIZE-1),PACKET_DATA_SIZE-1);
		//cout<<"[DEBUG] data is "<<data<<endl;
		//make packet
		pkt.set_type(type);
		//pkt.put_obj(data + t * PACKET_DATA_SIZE,sizeof(data));
		pkt.put_obj(data,sizeof(data));
		cout<<"[DEBUG] data is "<<(char *)(pkt.get_data())<<endl;
		// send the file through socket
		sock->my_send(&pkt);
    }
	cout<<"[Server] pkt data sent finish\n";
}

static void display_list( list<string> &l)
{
	cout << "[Server] list is ";
	for (auto & val : l)
	{
		cout<<val<<", ";
	}
	cout<<endl;
	return;
}


server_c::server_c()
{
	cout<<"please use the constructor with parameters!"<<endl;
}

server_c::server_c(std::string & torrent_folder_path)
{
	printf("[Server] Init server...\n");
    folder = torrent_folder_path;

    // read all files in this folder
	string result("");

	DIR * d = opendir( folder.c_str());
    if (d == NULL) 
    {
    	error_print("[ls] folder: "+folder+" doesn't exist! Please mkdir and restart the server!");
    	return;
    }
    for (dirent * de = NULL; (de = readdir(d)) != NULL;)
    {
    	string file_name(de->d_name);
		if (file_name == string(".") || file_name == string("..")) continue;
		string abs_path = folder + file_name;
		//cout<<"abs_path is "<<abs_path<<endl;
		//name_to_hash[get_first_line(abs_path)] = file_name;
		name_list.push_back(file_name);
    }
	cout<<"[Server] Init finish!"<<endl;
	//display_map(name_to_hash);
	display_list(name_list);
}

server_c::~server_c()
{
	out_sockets.clear();
	
}   


void server_c::listen(int port)
{
	//thd_listen_to_peer_t * temp = (thd_listen_to_peer_t *) thd_listen_to_peer_obj;
	//class client_c * this_obj = temp -> this_obj;
	//char Pig = 'A';
	//socket_c * listening_socket = new socket_c(temp -> client_sock, Pig);
	socket_c * listening_socket = new socket_c;
	packet_c * in_packet = new packet_c;
	packet_t * pkt_buf = new packet_t;
	//
	//file_c * target_file;
	//printf("going into while loop in listen_to_peer_func\n");
	listening_socket -> my_listen(port);
	while(true)
	{
		int client_socket = listening_socket -> my_accept();
		if (client_socket < 0)
		{
			//printf("ADDDDDDDDDDDD\n");
			sleep(1);
			continue;
		}

		do{
			if (listening_socket -> receive_single_pkt(*pkt_buf) == 0)
				break;
			in_packet -> load_packet(pkt_buf);

			// AREA of code for debugging
			//std::cout << "in pkt type::: " << in_packet -> get_type() << std::endl;
			//
		}while(in_packet -> get_type() == 'A');

		packet_c *pkt = new packet_c;
		pkt -> load_packet(pkt_buf);

		if (in_packet -> get_type() == PKT_TYPE_GET_TORRENT_LIST)
		{
			socket_c * listening_socket2 = new socket_c(client_socket);
			send_torrent_list(*pkt, listening_socket2);
		}
		else if (in_packet -> get_type() == PKT_TYPE_GET_TORRENT)
		{
			socket_c * listening_socket2 = new socket_c(client_socket);
			send_torrent(*pkt, listening_socket2);
		}
		else if (in_packet -> get_type() == PKT_TYPE_UP_TORRENT)
		{
			socket_c * listening_socket2 = new socket_c(client_socket);
			update_torrent_list(*pkt, listening_socket2);
		}

	}
	//this_obj -> listen_flag = 0;	// error occured while listening
	//delete temp;
	return;
	/*
	listen_socket = new socket_c();
	
	if (listen_socket->my_listen(port) == false)
	{
		error_print("[listen] listen failed");
		delete listen_socket;
		return;
	}
	packet_t * pkt_buf = new packet_t;
	packet_c *pkt = new packet_c;	// TODO: in this case, when pkt is passed to another thread, in the next loop, pkt will be changed. Will this affect the original pkt? If so, we might have to use a list to manage all these packet_c, instead of using only one pkt.
	int indicator = 0;
	printf("[Server] [listen] start while loop\n");
	while (1)
	{
		printf("[Server] [listen] waiting to accept a connection...\n");
		int client_sock = listen_socket->my_accept();
		printf("[Server] [listen] accept a new connection\n");
	    listen_socket->receive_single_pkt(*pkt_buf);
		pkt->load_packet(pkt_buf);
	    socket_c * thread_sock = (socket_c *) malloc( sizeof( socket_c)); 
	    thread_sock -> client_sock = client_sock;
		cout<<"[Server] [Listen] Receive one pkt: type is "<<pkt->get_type()<<endl;
		switch (pkt->get_type())
		{
		    // client want to upload a torrent
   	     	case PKT_TYPE_UP_TORRENT: update_torrent_list(*pkt,thread_sock);
				indicator = 0;
				break;

			// client wants to read the torrent list
       		case PKT_TYPE_GET_TORRENT_LIST: send_torrent_list(*pkt, thread_sock);
				indicator = 0;
				break;

			// client wants to get a torrent file
			case PKT_TYPE_GET_TORRENT: send_torrent(*pkt, thread_sock);
				indicator = 0;
				break;

			// non-matching torrent
			default: error_print("[Server] [listen] Unsuppported packet type");
				indicator = 1;
				break;
		}
		if (indicator == 0)
		{
			//out_sockets.push_back(*thread_sock);
	        //listen_socket = new socket_c(client_sock);
	        // TODO: in this case, the listen port has to change all the time?
			//if (listen_socket->my_listen(port) == false)
			//error_print("[Server] [Listen] socket listen and accept failed");
        }
        bzero(pkt_buf->data,PACKET_DATA_SIZE);
		bzero(pkt_buf->type,PACKET_CTR_SIZE);
	}
	delete listen_socket;
	delete pkt_buf;
	*/
}

static void * send_torrent_list_func(void * obj)
{
	printf("[Server] [send_torrent_list_func] start!\n");
	struct pthread_arg_packet_socket * arg = (struct pthread_arg_packet_socket *)obj;
	packet_c *pkt = arg->packet;
	socket_c *sock = arg->sock;
	server_c * server = (server_c *)arg->this_ptr;
	
	// get all torrent names
	string name_str;
	string torrent_names("");

	DIR * d = opendir( server->folder.c_str());
    if (d == NULL) 
    {
    	error_print("[ls] folder: "+server->folder+" doesn't exist!");
    	return NULL;
    }
    for (dirent * de = NULL; (de = readdir(d)) != NULL;)
    {
    	string file_name(de->d_name);
		if (file_name == string(".") || file_name == string("..")) continue;
		string abs_path = server->folder + "/"+file_name;
		torrent_names = torrent_names + get_first_line(abs_path) + "\n";
    }

	cout<<"Torrent names are "<<endl<<torrent_names<<endl;
	// make char * data and send
	make_char_pkt_and_send(sock, torrent_names.c_str(), torrent_names.size(), PKT_TYPE_TORRENT_LIST);
	
    // make the EOF
	packet_c pkt_eof;
	pkt_eof.set_type(PKT_TYPE_END);
	sock->my_send(&pkt_eof);

	//server->out_sockets.remove(*sock);
	//delete sock;
	delete pkt;
	delete arg;
	printf("[Server] send_torrent_list_func End\n");
	//pthread_exit(NULL);
	return NULL;
}



// receive one packet. pkt: PKT_TYPE_GET_TORRENT_LIST; pkt content: empty
// server send send packet: pkt: PKT_TYPE_TORRENT_LIST. pkt content: char *. char * end in '0'
// server send last pkt: 'X'	
int server_c::send_torrent_list(packet_c &pkt, socket_c * sock)
{

	pthread_t * tid = new pthread_t;  // thread ID
	//pthread_attr_t attr; // thread attribute
 
	// set thread detachstate attribute to DETACHED 
	//pthread_attr_init(&attr);	
	//pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
 
 	struct  pthread_arg_packet_socket* pthread_arg = new pthread_arg_packet_socket;
 	pthread_arg->packet = &pkt;
	pthread_arg->sock = sock;
	pthread_arg->this_ptr = this;
	// create the thread 
	if (pthread_create(tid, NULL, send_torrent_list_func, (void *)pthread_arg) != 0)
	{	
		error_print("error when create pthread function: send_torrent_list");
		return -1;
	}
	else
		printf("Successfully created send_torrent_list func thread\n");	
	return 0;
}


// Note: client use port_a to connect to server's port_listen. In this case the server shoud use a different port to connect to a different port of client? In other case, do we have to use a new socket? If not, using multi-threading is meaningless, as all comes out from the same port and we have to use mutex to lock client sock and server sock
// server get one pkt: PKT_TYPE_GET_TORRENT. content: char *, with '\0'.
// server send several pkt: PKT_TYPE_TORRENT. content: char *. last pkt '\0' in the middle of content
// server send last pkt: 'x'
// server send last pkt: PKT_TYPE_TR_SHA1
static void * send_torrent_func(void * obj)
{
	printf("[Server] [send_torrent_func] start\n");
	struct pthread_arg_packet_socket * arg = (struct pthread_arg_packet_socket *)obj;
	packet_c *pkt = arg->packet;
	socket_c *sock = arg->sock;
	server_c * server = (server_c *)arg->this_ptr;
	
	char * file_name = (char *) (pkt->get_data());
	printf("[Server] [send_torrent_func] get torrent name %s\n",file_name);
	//char file_name[sizeof(uint32_t)*5];
	//bzero(file_name,sizeof(uint32_t)*5);
	//sprintf(file_name,"%lu", server->name_to_hash[string(file_short_name)]);
	//memcpy(file_name,(server->name_to_hash[string(file_short_name)]).c_str(), sizeof(uint32_t)*5 );
	
	//printf("FIND ??? : %d\n", server->name_to_hash.count(string(file_short_name)));
	//printf("[Server] [send_torrent_func] torrent name is %s\n", file_name);
	string abs_path((server->folder + string(file_name)));
	size_t size = get_file_size(abs_path.c_str());
	FILE *f = fopen( abs_path.c_str(),"r");
	char* data = new char[size];
	memset(data,0,size);
	fread(data, sizeof(char), size, f);
	printf("The data we received is \n%s\n", data);
   	make_char_pkt_and_send(sock, data, size, PKT_TYPE_TORRENT);
	
	// make the EOF
	packet_c pkt_eof;
	pkt_eof.set_type(PKT_TYPE_END);
	sock->my_send(&pkt_eof);

	// make check_sum
	packet_c pkt_check_sum;
	pkt_check_sum.set_type(PKT_TYPE_TR_SHA1);
	uint32_t hash_result[5];
	char_str_to_hash(data, sizeof(data), hash_result);
	pkt_check_sum.put_obj((void *)&hash_result,sizeof(hash_result) );
	sock->my_send(&pkt_check_sum);

	//server->out_sockets.remove(*sock);
	delete[] data;
	delete arg;
	delete pkt;
	delete sock;
	printf("[Server] [send_torrent_func] end\n");
	return NULL;
}


int server_c::send_torrent(packet_c &pkt, socket_c * sock)
{
	pthread_t      tid;  // thread ID
	pthread_attr_t attr; // thread attribute
 
	// set thread detachstate attribute to DETACHED 
	pthread_attr_init(&attr);	
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
 
	struct  pthread_arg_packet_socket * pthread_arg = new pthread_arg_packet_socket;

	pthread_arg->packet = &pkt;
	pthread_arg->sock = sock;
	pthread_arg->this_ptr = this;
	
	// create the thread 
	if (pthread_create(&tid, &attr, send_torrent_func, (void *)pthread_arg) != 0)
	{	
		error_print("error when create pthread function: send_torrent_list");
		return -1;
	}
	else
		printf("Successfully created send_torrent_list func thread\n");	
	return 0;
}


// Whether to generate a new packet?
// server receive First packet:  pkt content: torrent_up_t; pkt: UP_TORRENT; start a new thread to send the torrent to the clients
// server receive second last pkt: 'X'
// server receive last pkt: 'SHA1'
int server_c::update_torrent_list(packet_c &pkt, socket_c *sock)
{
	printf("[Server] [update_torrent_list] start\n");
	struct torrent_up_t * r = (struct torrent_up_t *)(pkt.get_data());

	//TODO: note please add the hash as the name of torrent file
	char *tmp_name = new char [FILE_NAME_LIMIT];
	memcpy(tmp_name,r->tr_file_name,FILE_NAME_LIMIT);
	string str_for_name = string(tmp_name);

	// update the torrent list in memory
	string new_path = folder + str_for_name;
	
	// write the torrent file to disk
	ofstream new_torrent;
	//printf("check point 1\n");
	new_torrent.open(new_path.c_str());
	//printf("check point 2\n");
	char torrent_data [PACKET_DATA_SIZE];
	bzero(torrent_data,PACKET_DATA_SIZE);

	torrent_up_t * torrent = NULL;
	packet_c *tmp_pkt = &pkt;
	packet_t * pkt_buf = new packet_t;	
	//list<packet_c *> p_list;
	//list<packet_t *> buf_list;
	//p_list.push_back( tmp_pkt);
	//printf("check point 3\n");
	while (tmp_pkt->get_type() == PKT_TYPE_UP_TORRENT)
	{
		
		torrent = ((torrent_up_t *)(tmp_pkt->get_data()));
		//printf("trying to copy %u in MEMCPY\n", torrent->tr_data_size); 
		memcpy(torrent_data, torrent->tr_data, torrent->tr_data_size);
		new_torrent<<torrent_data;	
		//pkt_buf = new packet_t;
		sock->receive_single_pkt(*pkt_buf);
		//buf_list.push_back(pkt_buf);
		//tmp_pkt = new packet_c;
		tmp_pkt->load_packet(pkt_buf);
		bzero(torrent_data,PACKET_DATA_SIZE);
		//p_list.push_back( tmp_pkt);
	}

	if ( tmp_pkt->get_type()  !=  PKT_TYPE_END) 
	{
		return error_print ("update torrent pkg is not eof!");
	}
	new_torrent.close();

	//pkt_buf = new packet_t;
	sock->receive_single_pkt(*pkt_buf);
	//tmp_pkt = new packet_c;
	tmp_pkt->load_packet(pkt_buf);

	if ( tmp_pkt->get_type()  !=  PKT_TYPE_TR_SHA1) 
	{
		return error_print ("update torrent pkg is not SHA!");
	}
	//size_t size = get_file_size(new_path.c_str());
	
	uint32_t hash_torrent_file[5];
	calc_sha1_for_file(new_path, hash_torrent_file);
	/*FILE *f = fopen(new_path.c_str(),"r");
	char* data = new char[size];
	fread(data, sizeof(char), size, f);
	SHA1 sha1_for_file;
	uint32_t hash_torrent_file[5];
	char_str_to_hash(data, sizeof(data), hash_torrent_file);*/

	string str_for_hash_code;
	uint32_to_string(str_for_hash_code, hash_torrent_file, 5);
	//printf("str_for_hash_code = %s\n", str_for_hash_code.c_str());
	string uploaded_tr_hash;
	uint32_to_string(uploaded_tr_hash, (uint32_t *) tmp_pkt->get_data(), 5);// = string((char *) tmp_pkt->get_data());
	if (str_for_hash_code == uploaded_tr_hash) 
	{
		printf("[Server] Successfully uploaded file %s\n", tmp_name);
	}
	else
	{
		error_print("[Server] upload failed!");
	}
	name_list.push_back(str_for_name);
	display_list(name_list);
	//out_sockets.remove(*sock);
	//delete [] data;
	delete [] tmp_name;
	delete tmp_pkt;
	delete pkt_buf;
	delete sock;
	printf("[Server] [update_torrent_list] end\n");
	return 0;
}


static void calc_sha1_for_file(const std::string & the_file_path, uint32_t * its_sha1)
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
