#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};

    // Read normal data
    int valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (valread <= 0) {
        perror("Failed to receive data from client.\n");
        close(client_socket);
        return;
    }

    std::string received(buffer);
    std::cout << "Received: " << received << std::endl;

    if (received.find("NORMAL_DATA:") == 0) {
        std::string response = "SERVER_ACK:" + received.substr(strlen("NORMAL_DATA:"));
        send(client_socket, response.c_str(), response.length(), 0);
    }

    else if (received == "SEND_URGENT_REQUEST") {
        char oob_data;
        int oob_result = recv(client_socket, &oob_data, 1, MSG_OOB);
        if (oob_result == 1) {
            std::string response = "SERVER_URGENT_ACK:";
            response += oob_data;
            send(client_socket, response.c_str(), response.length(), 0);
        } else {
            std::string response = "SERVER_NO_URGENT_DATA";
            send(client_socket, response.c_str(), response.length(), 0);
        }
    }

    else {
        std::string response = "SERVER_UNKNOWN_COMMAND";
        send(client_socket, response.c_str(), response.length(), 0);
    }

    close(client_socket);
    std::cout << "Closed client connection.\n" << std::endl;
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

   
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    memset(address.sin_zero, 0, sizeof(address.sin_zero));

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
    std::cerr << "Invalid address" << std::endl;
    return 1;}

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "setsockopt failed" << std::endl;
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed" << std::endl;
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_socket < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        std::cout << "New client connected" << std::endl;
        handle_client(client_socket);
    }

    close(server_fd);
    return 0;
}
