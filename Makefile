all: user server testserver testclient

user: user.cpp
	g++ user.cpp -o user

server: server.cpp
	g++ server.cpp -o server

testserver: filetransferserver.cpp
	g++ filetransferserver.cpp -o testserver

testclient: filetransferclient.cpp
	g++ filetransferclient.cpp -o testclient