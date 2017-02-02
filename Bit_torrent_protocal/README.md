#Bit Torrent Protocal
### 1.  Class Definition
 
#### a) Packet

	class packet_c
	{
	public:
		packet_c();
		packet_c(char type, void * obj, int length); // construct a packet from a object 
		~packet_c();
		set_type(const char type);               // set the type of the empty object
		put_obj(const void * obj);               // put an object into the data
		make_packet(void * packet_str);          // when receive a socket, make an packet from this socket's result object
		void * parse_packet(packet_c * pkt);     // extract data from the coming packet, assuming the type of this packet is set in pkt_type
		char get_type(void);                     // cast the first byte of packet to in order to know how to   parse.
	        
	private:
		char pkt_type;   // The type of data it contains. ‘d’: data. ’r’: request. 
 		int pkt_length;  // The length of the packet
		char *data;      // data stream
	};

The packet class is an abstract wrapper of any data object, used as input of sockets. The first byte, pkt_type,
 indicates the type. Generally there are two types, data and request. Data is the data to be transmitted between peers. Requests are different actions to be executed. Different types of objects will be packed in different ways into the packet. Then the packet can be send as sockets. Similarly, on the receiver side, they will be cast and then parsed according to the packet’s type “pkt_type”. The data inside the packet is dynamically allocated, the length of which is pt_length.   

#### b)  Socket
	struct connection_info_t   // the wrapper of ip and port
	{
		char * ip;
		int port_num;
	};
	
	// Note: the detailed parameter are not written, please design by yourself
	// Note: it will be built on packet_c
	
	class socket_lis_c
	{
	public:
		socket_lis_c();
		~socket_lis_c();
		int socket_setup(packet_c, connection_info_t recv, …);  // set up the socket from port number, object packet, and so on	
		int (client_sock) listen_and_accept();   // open the port for listening
		send(packet_c & packet); 				     // send the socket of packet, make sure that the receiver has sent back a confirm
		packet_c receive();                                   // read data from the pipe of this socket, and return a packet. Also, send a confirm to the sender 
		close();
	private:
		void set_port(int port_num);
		int port;
		int c, socket_desc;
		struct sockaddr_in client;
	};
The socket type is a wrapper of the socket api, in order to make it easy to use, in sending, setting up, and receiving. Also, the socket class will be made from packet object. 

#### c) File

	struct peer_t               // The status of an ip.
	{
		bool isConnected;
		connection_info_t conInfo;
		char * pieces_status;
	};
	
	struct piece_t              // a piece in the file
	{
		string hash;
	long length;    
		long long offset;       // offset in the file
	};
	
	class file_c                 
	{ 
	public:
		file_c();
		~file_c();
		file_c(char * file_path);        // create a file object given the file path
		unsigned long get_size(void);    // get the file size
		std::vector<piece_t> cut(void);  // get a vector of pieces
		void set_torrent_path(char * torrent_path); // used in downloading torrent from the server  
		bool gen_torrent(char * torrent_path, connection_info_t tracker_info); // create the torrent, according to the torrent path, the tracker’s ip, assuming that the file is well initialized
		void set_tracker(connection_info_t tracker_info);  // set the tracker of the file // Is it usefull??
		void upload_to_server(connection_info_t server_info); // update the torrent of the file to the server
		void update_peer_list_in_tracker(void); // add this machine’s ip, port, to the tracker. 
		void get_peer_list_from_tracker_broadcast(); // When tracker finds that peer list has changed, tracker will send a request to peers to update their peer list
		void download_from_peers(void); 
	private:
		char * file_path
		char * torrent_path;
		int pieces_num;
		string sha1sum_file;
		struct stat statbuff;
		std::vector<string> sha1sum_list;  // hash code list
		std::vector<peer_t> peer_list;     
		std::vector<piece_t> piece_info_list;
		char * pieces_status;       // the char array, indicating each pieces’ status, like being downloading, not exist, or finished. Like an bitmap.
		connection_info_t tracker_info;
		void download_thread_func(void * peer_connection_info); // open a new thread for downloading, uploading
	};

File is the object managing file pieces, tracker, torrent, downloading, and uploading. Constructing from the file path, the file object will contain information about paths, data pieces, peers, tracker ip.  

#### d) Server

	struct send_torrent_msg_t    // ?
	{
		char * torrent_path;
		connection_info_t client;
	};
	
	class server_c
	{
	public:
		server_c();
		server_c(char * torrent_list_path);   // starting the server from the torrent folder
		~server_c();
		void send_torrent_list_func(); // new thread to send the torrent list to the clients
		void send_torrent_func(void * msg); // new thread to send the torrent to the clients
		char * get_torrent_path(string torrent_sha1sum); // get the torrent path from a hash code?
		void update_torrent_list(string torrent_sha1sum ??, char * torrent_path); // update the torrent list when a new torrent is uploaded. Write the torrent to the torrent_path
		void listen(int port); // listen to a port
	private:
		char * folder = "/torrents/";
		char * torrent_list_path;
		std::map<string torrent_sha1sum, char * torrent_path> torrent_list; // the torrent list in memory will be a map: the key is the hash code of torrent files, the value is the list of machine ids
		void write_torrent_file(packet_c torrent_packet);
	};

#### e) Tracker

	class tracker_c
	{
	public:
		tracker_c();
		tracker_c(char * peer_list_path); // read the peer list file and start the tracker 
		~tracker_c();
		list<connection_info_t> get_peer_list(string sha1sum);
		void update_peer_list(string torrent_sha1sum, connection_info_t new_host); // add the hash code to the peer list in memory and on disk. Call broadcast_updated_peer_list() to tell other peers about this change.
	private:
		char * peer_list_path;
		std::map<string, list<connection_info_t>> peer_list_in_mem; // the map, key is the hash code of each torrent file, value is the list of peers.
		list<connection_info_t> broadcast_updated_peer_list(string torrent_sha1sum); // when one machine started download/upload a file, it will tell tracker. Then tracker will broadcast to all other peers.
	};


#### f) Client
// Client will act as the top level, built upon packet_c

	class client_c
	{
	public:
		client_c();
		~client_c();
	       packet_c listen(); // listen to the port and distribute task to the file map. This will call functions in file class
	       //file * get_file(packet_c, char * file_path); // get the file object from the coming torrent
	        download_file_func(char * torrent_path, char * file_path); // new thread to download the filr,given the torrent path
		download_torrent_func(char * torrent_path); // new thread to get a torrent from the server., and write to the disk
	private:
		std::map<string,file_c> file_map;
	};

### 2. Step flow of import features
####Create and Uploading a torrent
1) Construct a file_c object from the path of the file, which calls cut() and fill the vector of pieces in file_c.
2) hash each piece to generate the torrent in generate_torrent()
3) wrap the torrent in the packet_c, and send as socket to server’s port. Server will call update_torrent_list(). 
4) hash the torrent file and pack and send to tracker. Tracker will call update_peer_list().
 
#### Downloading a torrent
1) Client call get_torrent() to send a packet of request to ask for torrent list. Server get this packet, and call send_torrent_list_fucn(), send _torrent_func() to send the torrent.
2) Client contruct a new file object, and write the torrent to the torrent folder on disk.
3) Peer update the peer list, and broadcast to all existing peers

####  Downloading files
1) Tracker add the machine ip to the peer list and broadcast to others
2) Machine contact each peer to get the current pieces each peer has
3) For each piece, choose a peer and start a new thread for downloading. If the timeout passed and doen’t get a reply, switch to a new machine for this piece.

