#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 1024

int main() {
    int sockfd, connfd;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Server address and port
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Bind socket to address and port
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        return -1;
    }

    // Listen for incoming connections
    if (listen(sockfd, 5) < 0) {
        perror("Listen failed");
        return -1;
    }

    // Accept incoming connection
    if ((connfd = accept(sockfd, (struct sockaddr*)&client_addr, &addr_len)) < 0) {
        perror("Accept failed");
        return -1;
    }

    // Receive file from client
    std::ofstream receivedFile("duck", std::ios::binary); // Replace with the desired file extension
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytesRead;

    while ((bytesRead = recv(connfd, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
        receivedFile.write(buffer, bytesRead);
    }

    receivedFile.close();
    close(connfd);
    close(sockfd);

    std::cout << "File received successfully." << std::endl;

    return 0;
}
