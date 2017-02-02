dependencies_h = const_values.h packet_c.h socket_c.h common.h sha1.h file_c.h 
dependencies_cpp =  packet_c.cpp  socket_c.cpp common.cpp sha1.cpp file_c.cpp
dependencies_o = packet_c.o socket_c.o common.o sha1.o file_c.o
CC = clang++
FLAGS = -Wall -lpthread -std=c++11 -g


all: server tracker client 


server: server.o
	${CC} server_c.o run_server.o ${dependencies_o} -o server ${FLAGS}

tracker: tracker.o
	${CC} tracker_c.o  run_tracker.o ${dependencies_o} -o tracker ${FLAGS}

client: client.o
	${CC} client_c.o run_client.o ${dependencies_o} -o client ${FLAGS}

tracker.o: ${dependencies_h} ${dependencies_cpp} tracker_c.h tracker_c.cpp run_tracker.cpp
	${CC} -c ${dependencies_cpp} tracker_c.cpp run_tracker.cpp ${FLAGS}

server.o: ${dependencies_h} ${dependencies_cpp} server_c.h server_c.cpp run_server.cpp
	${CC} -c ${dependencies_cpp} server_c.cpp run_server.cpp ${FLAGS}

client.o: ${dependencies_h} ${dependencies_cpp} client_c.h client_c.cpp run_client.cpp
	${CC} -c ${dependencies_cpp} client_c.cpp run_client.cpp ${FLAGS}

clean:
	rm -f *o
