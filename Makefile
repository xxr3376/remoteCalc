all:server client

server:server.cpp
	g++ $^ -o $@ -lpthread

client:client.cpp
