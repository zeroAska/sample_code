#include "server_c.h"
#include <string>
int main(int argc, char const *argv[])
{
	int port = atoi(argv[1]);
	std::string torrent_path = "./t/";
	server_c server(torrent_path);
	server.listen(port);
	return 0;
}
