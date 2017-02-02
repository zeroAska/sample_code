#include "tracker_c.h"
#include <string>
int main(int argc, char const *argv[])
{
	/* code */

	int port = atoi(argv[1]);
	std::string s = "./peer_list/";
	tracker_c tracker(s);
	tracker.listen(port);
	return 0;
}
