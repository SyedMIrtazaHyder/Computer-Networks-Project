#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>

//Going to the DNS server to get the IP and Port number of the client having the file
//Can maybe optimize to only show files that user himself doesnt have
#define SERVER_IP "127.0.0.1" //change this so can communicate with other machines as well
#define SERVER_PORT 8080     
#define MAX_BUFFER_SIZE 1024 //1Kb 

using namespace std;
namespace fs = std::filesystem;

vector<string> splitString(const string&);

class User{
    private:
    string username;
    string directoryPath;
    vector<string> filenames;

    public:
    int PORT;
    // Default constructor
    User() {
        cout << "Enter username: ";
        getline(std::cin, username);

        cout << "Enter directory path: ";
        getline(std::cin, directoryPath);

        while (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) { //checking if dir exists or not
            cerr << "Invalid directory path or directory doesn't exist." << endl;
            cout << "Enter directory path: ";
            getline(std::cin, directoryPath);
        }
        setFilenamesInDir(directoryPath);

        cout << "Enter sending PORT: ";
        cin >> PORT;
    }

    User(const string& username,const string& path, int PORT){
        this->username = username;
        this->directoryPath = path;
        this->PORT = PORT;
        if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
            cerr << "Invalid directory path or directory doesn't exist." << std::endl;
            // You might want to handle this error condition appropriately
            // For example, you could re-prompt the user for a valid directory path
        }
        else{
            setFilenamesInDir(this->directoryPath);
        }
    }

    User(const User& user){
        this->username = user.username;
        this->PORT = user.PORT;
        this->directoryPath = user.directoryPath;
        if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
            cerr << "Invalid directory path or directory doesn't exist." << std::endl;
            // You might want to handle this error condition appropriately
            // For example, you could re-prompt the user for a valid directory path
        }
        else{
            setFilenamesInDir(this->directoryPath);
        }
    }

    //setters and getters
    string getUsername(){
        return username;
    }

    void setUsername(string username){
        this->username = username;
    }

    string getDir(){
        return directoryPath;
    }

    void setDir(string dir){
        if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
            cerr << "Invalid directory path or directory doesn't exist." << std::endl;
            // You might want to handle this error condition appropriately
            // For example, you could re-prompt the user for a valid directory path
        }
        else
            this->directoryPath = dir;
    }

    void displayFilenamesInDir(){
        cout << "Files user has: " << endl;
        for(vector<string>::iterator i = filenames.begin(); i != filenames.end(); i++)
            cout << *i << endl;
    }

    vector<string> getFilenamesInDir(){
        return filenames;
    }

    string getFilenamesInDirAsString(){
        string files = "";
        for(vector<string>::iterator i = filenames.begin(); i + 1 != filenames.end(); i++) //going till second last element
            files += *i + ",";
        files += *(filenames.end() - 1); //adding last filename, so , does not cause problems when deciphering on server side
        return files;
    }

    void setFilenamesInDir(const string& folderPath) {
        if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
            cerr << "Invalid directory path or directory doesn't exist." << endl;
            return;
        }

        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (fs::is_regular_file(entry.path())) {
                filenames.push_back(entry.path().filename());
                //cout << entry.path().filename() << endl;
            }
        }
    }

    //Misc Functions
    void displayUserInfo() {
        cout << "Username: " << username << endl;
        cout << "Directory Path: " << directoryPath << endl;
    }

    static void* sendFile(void* args){ //giving it string array which has following pattern {fd, filename}
        string* ptr = static_cast<string*>(args);
        int fd = stoi(*ptr);
        string filename = *(ptr + 1);
        // Open file to send
        ifstream fileToSend("./user1/nahar", std::ios::binary); // Replace with the file name and extension
        if (!fileToSend.is_open()) {
            cerr << "Unable to open the file." << endl;
            return (void*) -1;
        }

        // Send file content
        char buffer[MAX_BUFFER_SIZE];
        ssize_t bytesRead;

        while ((bytesRead = fileToSend.readsome(buffer, MAX_BUFFER_SIZE)) > 0) {
            send(fd, buffer, bytesRead, 0);
        }

        fileToSend.close();
        cout << "File sent successfully." << endl;
        return (void*) 0; //so successuly file transmission
    }

    void serverConnect(){//Opening a TCP socket with the server to reliably tell the IP and data to the server
        intptr_t fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in s_addr;
    	s_addr.sin_family	= AF_INET;
	    s_addr.sin_port		= htons(SERVER_PORT);
	    inet_aton(SERVER_IP, &s_addr.sin_addr);

        if (connect(fd, (struct sockaddr*)&s_addr, sizeof(s_addr)) == -1) { //connecting to DNS server
            perror("Connect failed on socket : ");
            exit(-1);
    	}

        string myFiles = "HAS," + username + ","+ to_string(PORT) + "," + getFilenamesInDirAsString();//First message to server sharing all files it has
        send(fd, myFiles.c_str(), strlen(myFiles.c_str()), 0);
        cout << "Sent Data to Server" << endl;
        close(fd); //closing connection after sending data
    }

    void getFileFromServer(string& filename)
    {
        intptr_t fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in s_addr;
    	s_addr.sin_family	= AF_INET;
	    s_addr.sin_port		= htons(SERVER_PORT);
	    inet_aton(SERVER_IP, &s_addr.sin_addr);
        char buffer[1000];

        if (connect(fd, (struct sockaddr*)&s_addr, sizeof(s_addr)) == -1) { //connecting to DNS server
            perror("Connect failed on socket : ");
            exit(-1);
    	}

        string myFiles = "GET," + filename;//First message to server sharing all files it has
        send(fd, myFiles.c_str(), strlen(myFiles.c_str()), 0);
        cout << "Sent Data to Server" << endl;
        bzero(buffer, strlen(buffer));
        if (recv(fd, buffer, 1000, 0) > 0)
            cout << buffer << endl;
        else{
            cerr << "File does not exist" << endl;
            close(fd);
            return;
        }
        close(fd); //closing connection after sending data
        vector<string> cliINFO = splitString(string(buffer));
        connectToPeer(cliINFO[0], stoi(cliINFO[1]), filename);
    }

    void getAllFilenamesFromServer()
    {
        intptr_t fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in s_addr;
    	s_addr.sin_family	= AF_INET;
	    s_addr.sin_port		= htons(SERVER_PORT);
	    inet_aton(SERVER_IP, &s_addr.sin_addr);
        char buffer[1000];

        if (connect(fd, (struct sockaddr*)&s_addr, sizeof(s_addr)) == -1) { //connecting to DNS server
            perror("Connect failed on socket : ");
            exit(-1);
    	}

        string myFiles = "ALL";//First message to server sharing all files it has
        send(fd, myFiles.c_str(), strlen(myFiles.c_str()), 0);
        cout << "Sent Data to Server" << endl;
        if (recv(fd, buffer, 1000, 0))
            cout << buffer << endl;
        vector<string> data = splitString(string(buffer));
        //cout << "Connecting to " << data[2] << endl;
        //connectToPeer(data[0], stoi(data[1]));
        //close(fd); //closing connection after sending data
    }

    void connectToPeer(string pIP, int pPORT, const string& filename){
        intptr_t fd = socket(AF_INET, SOCK_STREAM, 0); //again TCP as in UDP no guarentee of packet order
        struct sockaddr_in p_addr;
    	p_addr.sin_family	= AF_INET;
	    p_addr.sin_port		= htons(pPORT);
	    inet_aton(pIP.c_str(), &p_addr.sin_addr);

        if (connect(fd, (struct sockaddr*)&p_addr, sizeof(p_addr)) == -1) { //connecting to client having the file we want
            perror("Connect failed on socket : ");
            exit(-1);
    	}

        send(fd, filename.c_str(), strlen(filename.c_str()), 0);
        cout << "Sent Data to Clients" << endl;

        //Recieving file
        std::ofstream receivedFile(directoryPath + "/" + filename, std::ios::binary); //recieving file as bin chunks and saving in directory
        char buffer[MAX_BUFFER_SIZE];
        ssize_t bytesRead;

        while ((bytesRead = recv(fd, buffer, MAX_BUFFER_SIZE, 0)) > 0)
            receivedFile.write(buffer, bytesRead);
        receivedFile.close();
        close(fd); //closing connection after sending data
    }

    std::uintmax_t getFileSize(string& filename) { //getting file size (in BYTES) so we know how much data we are sending.
        filename = directoryPath + "/" + filename;
        if (!fs::exists(filename) || !fs::is_regular_file(filename)) {
            std::cerr << "File doesn't exist or is not a regular file." << std::endl;
            return 0;
        }
        return fs::file_size(filename);
    }
};
void* UI(void*);

int main() {
    User user1;
    user1.serverConnect();

    int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("Socket creation failed : ");
		exit (-1);	
	}

    //binding port for listening for any requests to send data
  	struct sockaddr_in p_addr;
	p_addr.sin_addr.s_addr	= INADDR_ANY;
	p_addr.sin_family	= AF_INET;
	p_addr.sin_port		= htons(user1.PORT);

    if (bind(fd, (struct sockaddr*)(&p_addr), sizeof(p_addr) ) == -1) {
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
    char buffer[1000];
    do{
        int UI_thread = pthread_create(&th, NULL, UI, (void*) &user1);
        struct sockaddr_in c_addr;
        socklen_t cliaddr_len = sizeof(c_addr);
        intptr_t connfd = accept(fd, (struct sockaddr*)&c_addr, &cliaddr_len);
		if (connfd > 0){
            bzero(buffer, strlen(buffer));//clearing buffer from any garbage it has
            //displayFileNames();
			cout << "Connection found" << endl;
            string arr[2] = {to_string(connfd), inet_ntoa(c_addr.sin_addr)};
            //Recieving msg from p2p client of what to send them.
            if (recv(fd, buffer, strlen(buffer), 0))
		        {
                    //buffer only has filename
                    string arr[2] = {to_string(connfd), string(buffer)};
                    int file_thread = pthread_create(&th, NULL, user1.sendFile, (void *) arr);
                }
		}
	} while (1);
    return 0;
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

void* UI(void* args){
    User* user = (User*) args;
    User user1(*user);
    int option;
    string filename;
    while(1){
        cout << "Choose option:\n1.View all files\n2. Get File\n0. Exit" << endl;
        cin >> option;
        switch (option){
            case 0:
                break;
            
            case 1:
                user1.getAllFilenamesFromServer();
                break;

            case 2:
                cin.ignore(numeric_limits<std::streamsize>::max(), '\n'); // Clear the input buffer
                cout << "Enter file to get: ";
                cin.clear();
                getline(cin, filename);
                user1.getFileFromServer(filename);
                break;

            default:
                continue;
        }

        if (option == 0)
            break;
    }
    //cout << file << " is " << user1.getFileSize(file) << " bytes" << endl;
    return (void*) 0;
}