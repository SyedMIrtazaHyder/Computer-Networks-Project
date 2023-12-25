#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;

    // Create TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Server address and port
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Replace with the server's IP

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    // Open file to send
    std::ifstream fileToSend("./user2/ProbabilisticRobotics.pdf", std::ios::binary); // Replace with the file name and extension
    if (!fileToSend.is_open()) {
        std::cerr << "Unable to open the file." << std::endl;
        return -1;
    }

    // Send file content
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytesRead;

    while ((bytesRead = fileToSend.readsome(buffer, MAX_BUFFER_SIZE)) > 0) {
        send(sockfd, buffer, bytesRead, 0);
    }

    fileToSend.close();
    close(sockfd);

    std::cout << "File sent successfully." << std::endl;

    return 0;
}
