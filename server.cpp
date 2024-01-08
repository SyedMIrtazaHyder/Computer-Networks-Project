//Will have a map, which will contain user mapped to the files they share.
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <sstream>
#include <arpa/inet.h>
#include <algorithm>

// Define your map and mutex to ensure thread safety

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

using namespace std;
multimap<string, vector<string>> filesWithIP; //files mapped with {IP, Port, Username}
vector<string> clients;
void* RecieveFromClient(void* args);
void  displayFileNames();
void  sendFileNames(int&);
vector<string> splitString(const string&);
string p2pClient(const string&); 
string getUser(const string&);
void deleteOnDisconnect(const string&);
bool isInMap(const string&, vector<string>&);

int main() {
	//initializing server
	intptr_t fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("Socket creation failed : ");
		exit (-1);	
	}
	
	struct sockaddr_in s_addr;
	s_addr.sin_addr.s_addr	= INADDR_ANY;
	s_addr.sin_family	= AF_INET;
	s_addr.sin_port		= htons(SERVER_PORT);

	if (bind(fd, (struct sockaddr*)(&s_addr), sizeof(s_addr) ) == -1) {
		perror("Bind failed on socket : ");
		exit(-1);
	}	
	
	int backlog = 4;
	if (listen(fd, backlog) == -1) {
		perror("listen failed on socket : ");
		exit(-1);
	}

	//initializing thread
	pthread_t th;	//thread handler, no of calls =  no. independent threads running at runtime 
	//can only thread if return type is void* and has single void* input
	void *result;
	void *tempArg;
	
	do{
		struct sockaddr_in c_addr;
		socklen_t cliaddr_len = sizeof(c_addr);
		intptr_t connfd = accept(fd, (struct sockaddr*)&c_addr, &cliaddr_len);

		if (connfd > 0){
            //displayFileNames();
			cout << "Connection found" << endl;
            string arr[2] = {to_string(connfd), inet_ntoa(c_addr.sin_addr)};
			int msg_thread = pthread_create(&th, NULL, RecieveFromClient, (void *) arr);
		}
	} while (1);
	return 0;
}

void* RecieveFromClient(void* args){
    string* ptr = static_cast<string*>(args);
	int connfd = stoi(*ptr);
    string cli_IP = *(ptr + 1);
	char buffer[1000];

	if (recv(connfd, buffer, 1000, 0))
        cout << buffer << endl;

    vector<string> data = splitString(string(buffer));
	string type = data[0]; //getting type of input
	
    if (type == "HAS"){
		string cli_name = data[1];
		string cli_PORT = data[2];
		vector<string> cli_data = {cli_IP, cli_PORT, cli_name};
		cout << type << " " << cli_name << " " << cli_PORT << endl;
		if (find(clients.begin(), clients.end(), cli_name) == clients.end()) // so user does not exist
			clients.push_back(cli_name);

        for(vector<string>::iterator i = data.begin()+3; i!=data.end(); i++) //as 1st 3 parts have sender info
			if (!isInMap(*i, cli_data))//for repeated files
            	::filesWithIP.insert(pair<string, vector<string>>(*i, cli_data)); //inserting file with client info on the server
    	//cout << ::filesWithIP.size() << endl;
    }

	else if (type == "ALL"){
		sendFileNames(connfd);
	}

	else if (type == "GET"){
		string fileToGet = data[1];
		string peerInfo = p2pClient(fileToGet);
		send(connfd, peerInfo.c_str(), strlen(peerInfo.c_str()), 0); //sending the details of the client that has the file
	}
	
	else if (type == "USERS"){
		string msg;
		 for (int i = 0; i < clients.size() - 1; i++) {
        	msg=msg+ clients[i]+",";
   		}
		msg += *(clients.end() - 1) + "*";
		send(connfd,msg.c_str(),strlen(msg.c_str()),0);		
	}
	
	else if (type == "WHO"){
		string userToGet = data[1];
		string peerInfo = getUser(userToGet);
		send(connfd, peerInfo.c_str(), strlen(peerInfo.c_str()), 0); //sending the details of the client file
	}
	
	else if (type == "EXIT"){
		int c = 0;
		for(; c < data[1].length(); c++)
            if (buffer[c] == '*')
                break;
		string exiting_user = data[1].substr(0, c-1);
		cout << "User: " << exiting_user << " has left..." << endl;
		deleteOnDisconnect(exiting_user);		
	}
	
	else{
		cout << "Invalid Command to Server..." << endl;
		char invalid[] = "Invalid Command to Server...";
		send(connfd, invalid, strlen(invalid), 0);
	}

	close(connfd);
	return (void*) 1;
}

void displayFileNames(){ //function to print all the files present in server at any given time
    cout << "Current Files:" << endl;
    for (const auto& pair : ::filesWithIP)
        cout << "Filename: " << pair.first << endl;
}

void sendFileNames(int& connfd){ //function to print all the files present in server at any given time
    string buffer = "";
    for (const auto& pair : ::filesWithIP) //loading buffer with all filenames
        buffer += pair.first + "\n";
    send(connfd, buffer.c_str(), strlen(buffer.c_str()), 0);//sending all filenames available to user
}

string getUser(const string& username){
	multimap<string, vector<string>>::iterator i = filesWithIP.begin();
    string out;
    for (; i!=filesWithIP.end();){
		if (i->second[2] == username){
			out=out+i->second[0]+","+i->second[1];
			return out;
			}
		else
			i++;	
	}
	cout<<"USERNOTFOUND"<<endl;
	return "";
}

string p2pClient(const string& filename){
	auto i = ::filesWithIP.find(filename);
	if (i != ::filesWithIP.end()){
		vector<string> client_info = i->second;//::filesWithIP[filename];
		string out = "";
		for (auto i2 = client_info.begin(); i2 != client_info.end() - 1; i2++)
			out += *i2 + ",";
		out += *(client_info.end() - 1);
		out += "*";
		return out;
	}
	cerr << "FILE NOT FOUND" << endl;
	return "";
}

// Function to delete entries when a client is disconnected
void deleteOnDisconnect(const string& clientID) {
	multimap<string, vector<string>>::iterator i = filesWithIP.begin();
    for (; i!=filesWithIP.end();){
		if (i->second[2] == clientID)
			i = filesWithIP.erase(i);
		else
			i++;	
	}
	cout << "Client " << clientID << " entries have been removed." << endl;
	//removing user from vector array
	::clients.erase(find(::clients.begin(), ::clients.end(), clientID));
	cout << "Client has disconnected" << endl;
}

bool isInMap(const string& filename, vector<string>& clientData){
	auto i = filesWithIP.equal_range(filename); //pair<start_iterator, end_iterator>, as multimap stores sorted keys so sequence == all enteries
	//type: pair<multimap<string, vector<string>>::iterator, multimap<string, vector<string>>::iterator>
	for(auto begin = i.first; begin != i.second; begin++)
		if (clientData == begin->second) //similar entry so no need to add another
			return true;
			//::filesWithIP.insert(pair<string, vector<string>>(filename, clientData));
	return false;
}


vector<string> splitString(const string& inputString) {
    vector<string> result;
    stringstream ss(inputString);
    string item;

    while (getline(ss, item, ',')) {
        result.push_back(item);
    }

    return result;
}
