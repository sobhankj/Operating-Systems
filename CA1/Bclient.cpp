#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <algorithm>
#include <fcntl.h>
#include <map>
#include <vector>
#include <csignal>

#define PORT 8080
#define BUFFER_SIZE 1024


int main(int argc , char*argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::string message = "Socket creation error\n";
        write(STDOUT_FILENO , message.c_str() , message.size());
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(std::stoi(argv[2]));

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        std::string message = "Invalid address / Address not supported\n";
        write(STDOUT_FILENO , message.c_str() , message.size());
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::string message = "Connection failed\n";
        write(STDOUT_FILENO , message.c_str() , message.size());
        return -1;
    }

    std::string message = "Connected to server\n";
    write(STDOUT_FILENO, message.c_str() , message.size());

    fd_set readfds;
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int activity = select(sock + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(sock, &readfds)) {
            int valread = read(sock, buffer, BUFFER_SIZE);
            if (valread == 0) {
                std::string message = "Server disconnected.\n";
                write(STDOUT_FILENO, message.c_str() , message.size());
                close(sock);
                break;
            } else {
                buffer[valread] = '\0';
                // std::string message = "Server: " + buffer + "\n";
                // write(STDOUT_FILENO, message.c_str() , message.size());
                std::cout << "Server: " << buffer << std::endl;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            std::string message;
            std::getline(std::cin, message);
            send(sock, message.c_str(), message.size(), 0);
        }
    }

    return 0;
}