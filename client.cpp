#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

void scenario1();
void scenario2();
void scenario3();
void scenario4();

int main() {
    std::cout << "Scenario 1: Normal data" << std::endl;
    scenario1();
    std::cout << "\nScenario 2: URGENT DATA REQUEST" << std::endl;
    scenario2();
    std::cout << "\nScenario 3: URGENT DATA without OOB" << std::endl;
    scenario3();
    std::cout << "\nScenario 4: Garbage" << std::endl;
    scenario4();
    return 0;
}

int create_and_connect_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed\n";
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed\n";
        close(sock);
        return -1;
    }

    return sock;
}

void scenario1() {
    int sock = create_and_connect_socket();
    if (sock < 0) return;

    const char* msg = "NORMAL_DATA:Hello";
    send(sock, msg, strlen(msg), 0);
    std::cout << "Sent: " << msg << std::endl;

    char buffer[BUFFER_SIZE] = {};
    int valread = recv(sock, buffer, BUFFER_SIZE, 0);
    if (valread > 0)
        std::cout << "Received: " << buffer << std::endl;

    close(sock);
}

void scenario2() {
    int sock = create_and_connect_socket();
    if (sock < 0) return;

    const char* request = "SEND_URGENT_REQUEST";
    send(sock, request, strlen(request), 0);
    std::cout << "Sent: " << request << std::endl;

    const char* urgent_byte = "U";
    send(sock, urgent_byte, 1, MSG_OOB);
    std::cout << "Sent urgent byte: " << urgent_byte << std::endl;

    char buffer[BUFFER_SIZE] = {};
    int valread = recv(sock, buffer, BUFFER_SIZE, 0);
    if (valread > 0)
        std::cout << "Received: " << buffer << std::endl;

    close(sock);
}

void scenario3() {
    int sock = create_and_connect_socket();
    if (sock < 0) return;

    const char* request = "SEND_URGENT_REQUEST";
    send(sock, request, strlen(request), 0);
    std::cout << "Sent: " << request << std::endl;


    char buffer[BUFFER_SIZE] = {};
    int valread = recv(sock, buffer, BUFFER_SIZE, 0);
    if (valread > 0)
        std::cout << "Received: " << buffer << std::endl;

    close(sock);
}

void scenario4() {
    int sock = create_and_connect_socket();
    if (sock < 0) return;

    const char* msg = "TEST_GARBAGE";
    send(sock, msg, strlen(msg), 0);
    std::cout << "Sent: " << msg << std::endl;

    char buffer[BUFFER_SIZE] = {};
    int valread = recv(sock, buffer, BUFFER_SIZE, 0);
    if (valread > 0)
        std::cout << "Received: " << buffer << std::endl;

    close(sock);
}
