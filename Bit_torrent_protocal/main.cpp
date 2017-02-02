#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h>
#include <stdio.h>
#include <iostream>
#include <sys/stat.h>
#include <stdint.h>
#include "sha1.h"
#include "const_values.h"
#include "file_c.h"
#include "packet_c.h"
#include "socket_c.h"
#include "client_c.h"

int main(int argc, char const *argv[])
{
	client_c a;
	//file_c b;
	
	if (argc == 1)
	{
		a.interface();
		/*
		connection_info_t tracker_info;
		strcpy(tracker_info.ip,"192.168.137.142");
		tracker_info.port_num = 8888;
		class file_c * one_file = new file_c("File2");
		one_file -> gen_torrent("File.tr", tracker_info);
		delete one_file;

		one_file = new file_c;
		one_file -> load_torrent("File3", "File.tr");
		delete one_file;
		//one_file -> update_peer_list_in_tracker();
		//one_file -> upload_to_server(tracker_info);
		//one_file -> get_peer_list_from_tracker();
		sleep(3);*/
	}
	else if (argc == 2)
	{
		//a.interface();
		//printf("returned!!!!\n");
		
		int port_lis = atoi(argv[1]);
		class socket_c my_socket;
		my_socket.listen_and_accept(port_lis);
		packet_t my_packet_buf;
		class packet_c my_packet;
		while(my_socket.receive_single_pkt(my_packet_buf) > 0)
		{
			my_packet.load_packet(& my_packet_buf);
			std::cout << "type::: " << my_packet.get_type() << std::endl;
			const sha1_and_mac_t * buf = (const sha1_and_mac_t *) my_packet.get_data();
			std::cout << "mac:: " << buf -> mac << std::endl;
		}
		printf("client disconnected\n");
		
	}
	else if (argc == 3)
	{
		connection_info_t temp;
		memcpy(temp.ip, argv[1], 20);
		temp.port_num = atoi(argv[2]);
		class socket_c my_socket(temp);
		my_socket.my_connect();
		class packet_c my_packet;
		my_packet.set_type('z');
		sha1_and_mac_t buf;
		mac_eth0(buf.mac);	
		my_packet.put_obj((void *) & buf, SHA1_LENGTH + 13);
		my_socket.my_send(& my_packet);
	}
	else if(argc == 4)
	{
		file_c my_f;
		my_f.load_torrent("File_new", "File.tr2");
		peer_t apeer;
		apeer.isConnected = 0;
		strcpy(apeer.conInfo.ip, "192.168.145.130");
		apeer.conInfo.port_num = 8889;
		apeer.pieces_status = new char[my_f.pieces_num];
		my_f.peer_list.push_back(apeer);
		printf("in it? %s:%d\n", apeer.conInfo.ip, apeer.conInfo.port_num);
		printf("really in it? %s:%d\n", my_f.peer_list[0].conInfo.ip, my_f.peer_list[0].conInfo.port_num);
		printf("checkingg~~~\n");
		my_f.download_from_peers();
		while(1);
	}
	return 0;
}
