#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <any>
#include <variant>
#include <thread>
#include <chrono>
#include <queue>
#include <ctime>

//Going to the DNS server to get the IP and Port number of the client having the file
//Can maybe optimize to only show files that user himself doesnt have
#define SERVER_IP "127.0.0.1" //change this so can communicate with other machines as well
#define SERVER_PORT 8080     
#define MAX_BUFFER_SIZE 1024 //1Kb 

using namespace std;
namespace fs = std::filesystem;

vector<string> splitString(const string&);
queue<string> messageBuffer;
void displayMessage();

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
        filenames.clear();//clearing any previous data
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
        close(fd);//closing connection after sending data
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
    
    void viewOnlineUsers(){
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

        bzero(buffer, strlen(buffer));
        string msg = "USERS";//First message to server sharing all files it has
        send(fd, msg.c_str(), strlen(msg.c_str()), 0);
        cout << "Sent Request" << endl;
        recv(fd, buffer, 1000, 0);
        cout << flush << buffer << endl;
    	close(fd);
    }
    
    void chat(string username){     
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

        string msg = "WHO," + username;
        send(fd, msg.c_str(), strlen(msg.c_str()), 0);
        cout << "Sent Request" << endl;
        bzero(buffer, strlen(buffer));
        if (recv(fd, buffer, 1000, 0) > 0)
            cout << buffer << endl;
        else{
            cerr << "User does not exist" << endl;
            close(fd);
            return;
        }
        close(fd); //closing connection after sending data
        vector<string> cliINFO = splitString(string(buffer));
        connectToPeerChat(cliINFO[0], stoi(cliINFO[1]), username);
    }
    
    void connectToPeerChat(string pIP, int pPORT, string& username){
        int fd = socket(AF_INET, SOCK_STREAM, 0); //again TCP as in UDP no guarentee of packet order

        struct sockaddr_in p_addr;
    	p_addr.sin_family	= AF_INET;
	    p_addr.sin_port		= htons(pPORT);
	    inet_aton(pIP.c_str(), &p_addr.sin_addr);

        if (connect(fd, (struct sockaddr*)&p_addr, sizeof(p_addr)) == -1) { //connecting to client for chat
            perror("Connect failed on socket : ");
            exit(-1);
    	}
        time_t now = time(0); // get current dat/time with respect to system.
		char* dt = ctime(&now); // convert it into string.

    	string mssg;
        cout << "Enter Message: ";
        getline(cin, mssg);
        mssg = "CHAT:" + string(dt, strlen(dt)) + "->" + this->username + ": " + mssg;
	    send(fd,mssg.c_str(),mssg.size()+1,0);
        close(fd);

      	// thread([&]() { //initating a thread to listen to the other chatter
        //     char buffer[1000];
        //     int bytesRead;
        //     while ((bytesRead = recv(fd, buffer, sizeof(buffer), 0)) > 0) {
        //         ::messageBuffer.push(string(buffer, strlen(buffer))); 
        //     }
        // }).detach();
    
        // while(true){
        //     char message[1000];
        //     cout << "Enter Message: ";
        //     cin >> message;
        //     if (!::messageBuffer.empty()){
        //         ::displayMessage();
        //     }
        //     if (strcmp(message, "/exit") == 0 || send(fd, message, strlen(message), 0) == -1){
        //         cout << "Exiting" << endl;
        //         close(fd);
        //         return;
        //     }
      	// }
    }
    
    void connectToPeer(string pIP, int pPORT, string& filename){
        int fd = socket(AF_INET, SOCK_STREAM, 0); //again TCP as in UDP no guarentee of packet order
        struct sockaddr_in p_addr;
    	p_addr.sin_family	= AF_INET;
	    p_addr.sin_port		= htons(pPORT);
	    inet_aton(pIP.c_str(), &p_addr.sin_addr);

        if (connect(fd, (struct sockaddr*)&p_addr, sizeof(p_addr)) == -1) { //connecting to client having the file we want
            perror("Connect failed on socket : ");
            exit(-1);
    	}
    	
        string peer_msg = "FILE:" + filename + "*"; //End of string car
        send(fd, peer_msg.c_str(), strlen(peer_msg.c_str()), 0);

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

    void userExit(){
        //Sending message to server that client is leaving hence remove the files it has
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

        string exitMsg = "EXIT," + username + "*";//Last message that server recieves from client
        send(fd, exitMsg.c_str(), strlen(exitMsg.c_str()), 0);
        cout << "Sent Exit Msg to Server" << endl;
        close(fd); //closing connection after sending data
    }

    void updateServer(){
        intptr_t fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in s_addr;
    	s_addr.sin_family	= AF_INET;
	    s_addr.sin_port		= htons(SERVER_PORT);
	    inet_aton(SERVER_IP, &s_addr.sin_addr);

        if (connect(fd, (struct sockaddr*)&s_addr, sizeof(s_addr)) == -1) { //connecting to DNS server
            perror("Connect failed on socket : ");
            exit(-1);
    	}

        cout << "Updating Server" << endl;
        setFilenamesInDir(directoryPath); //updating file names
        string myFiles = "HAS," + username + ","+ to_string(PORT) + "," + getFilenamesInDirAsString();//First message to server sharing all files it has
        send(fd, myFiles.c_str(), strlen(myFiles.c_str()), 0);
        close(fd); //closing connection after sending data
    }

};
//void* UI(void*);
bool UI(User&);
void sendFile(int&, const string&, const string&);
void* listener(void*);

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
    //as we just need 1 copy of this thread
    vector<any> connection_data = {fd, user1.getDir()};//maybe make this thread before main function
	int listening_thread = pthread_create(&th, NULL, listener, static_cast<void*>(&connection_data));	
    do{
        //fix display after sending thread finsihed, or do not display anything when sending thread runs...
	} while (UI(user1));    return 0;
}

vector<string> splitString(const string& inputString) {
    vector<string> result;
    stringstream ss(inputString);
    string item;

    while (getline(ss, item, ',')) {
        if (item.find("*") != item.npos){
            string new_item = "";
            for (int i = 0; item[i] != '*'; i++)
                new_item += item[i];
            item = new_item;
        }
        result.push_back(item);
    }
    return result;
}

void sendFile(int& connfd, const string& dir, const string& filename){
    //Sending file
    //int file_thread = pthread_create(&th, NULL, user1.sendFile, static_cast<void*>(&arr));
    //cout << "Sending: " << filename << endl;
                    
    // Open file to send    
    ifstream fileToSend(dir + "/" + filename, std::ios::binary); // Replace with the file name and extension
    if (!fileToSend.is_open()) {
        cerr << "Unable to open the file." << endl;
        return;
    }

    // Send file content
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = fileToSend.readsome(buffer, MAX_BUFFER_SIZE)) > 0)
        send(connfd, buffer, bytesRead, 0);

    fileToSend.close();
   
    //cout << "File sent successfully." << endl;
    close(connfd);
    return;
}

void displayMessage(){
    while(!::messageBuffer.empty()){
        cout << messageBuffer.front() << endl;
        ::messageBuffer.pop();
    }
}

// void startChatting(int& connfd){
//     // cout << "Enter 5 to start chatting with the user..." << endl;
//     // auto start = chrono::high_resolution_clock::now();
//     // while(!isChatting){
//     //     auto now = chrono::high_resolution_clock::now();
//     //     if (chrono::duration_cast<chrono::seconds>(now - start).count() > 10){//if doesnt responds in 10 seconds break
//     //         cout << "Rejecting Chatting Request" << endl;
//     //         // char exit_msg[] = "/exit";
//     //         // send(connfd, exit_msg, strlen(exit_msg), 0);
//     //         isChatting = false;
//     //         return;
//     //     }
//     // } //stuck in this loop until isChatting turned on
//     while(1){
//         thread([&]() {
//             int bytesRead;
//             char buffer[1000];
//             while ((bytesRead = recv(connfd, buffer, sizeof(buffer), 0)) > 0) {
//                 //To prevent dumb cout cin issues, going to simply put all msgs in buffer and display after cin
//                 ::messageBuffer.push(string(buffer, strlen(buffer)));
//             }
//         }).detach();
//         // while(true){
//         //     char message[1000];
//         //     cout << "Enter Message: ";
//         //     cin >> message;
//         //     // if (!::messageBuffer.empty()){
//         //     //     displayMessage();
//         //     // }
//         //     if (strcmp(message, "/exit") == 0 || send(connfd, message, strlen(message), 0) == -1){
//         //         cout << "Exiting" << endl;
//         //         close(connfd);
//         //         return;
//         //     }
//         // }
//         // isChatting = false;
//     }
//     return;
// }

bool UI(User& user1){
    //User* user = (User*) args;
    //User user1(*user);

    int option;
    string filename;
    string username;
    cout << "Choose option:\n1.View all files\n2. Get File\n3. View Online Users\n4. Send Message\n5. View Messages\n0. Exit" << endl;
    cin >> option;
    switch (option){
        case 0:
            user1.userExit();
            return false;
            
        case 1:
            user1.getAllFilenamesFromServer();
            break;

        case 2:
            cin.ignore(numeric_limits<std::streamsize>::max(), '\n'); // Clear the input buffer
            cout << "Enter file to get: ";
            cin.clear();
            getline(cin, filename);
            user1.getFileFromServer(filename);
            user1.updateServer();
            break;

        case 3:
            user1.viewOnlineUsers();
            break;

        case 4:
            cin.ignore(numeric_limits<std::streamsize>::max(),'\n');
        	cout<<"Enter Username: ";
        	cin.clear();
        	getline(cin,username);
            user1.chat(username);
            break;

        case 5:
            displayMessage();
            break;

        default:
            cout << "Invalid input..." << endl;
    }
    return true;
}

void* listener(void* args){
    vector<any> connection_data = *(static_cast<vector<any>*>(args));
    int fd = any_cast<int>(connection_data[0]);
    string dir = any_cast<string>(connection_data[1]);
    
    char buffer[1000];
    while(1){
        struct sockaddr_in c_addr;
        socklen_t cliaddr_len = sizeof(c_addr);
        int connfd = accept(fd, (struct sockaddr*)&c_addr, &cliaddr_len);
        if (connfd > 0){
            memset(buffer, 0, strlen(buffer));//clearing buffer from any garbage it has
            //displayFileNames();           
            int c = 0;
            if (recv(connfd, buffer, 1000, 0) > 0){ //removing terminating char from string
                //cout << "Connection Found" << endl;
                string msg = string(buffer, strlen(buffer));
                string msg_type = msg.substr(0,4);
                //cout << msg_type << endl;
                int c = 5;
                for(; c < strlen(buffer); c++)
                    if (buffer[c] == '*')
                        break;           
                //string msg(buffer, c);
                //cout << msg_type << " is " << msg << "\n" << msg.substr(5,c-5) << msg.substr(5,c-4) << endl;
                if (msg_type=="CHAT"){
                    cout << "New notification" << endl;
                    ::messageBuffer.push(msg.substr(5,c-5));
                    close(connfd);
                }
                else if (msg_type=="FILE")
                    sendFile(connfd,dir,msg.substr(5,c-5));
            }
            //close(connfd);        
        }
    }
    return (void*) 1;
}