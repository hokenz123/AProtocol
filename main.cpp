#include "frag.hpp"
#include <bits/stdc++.h>
#include <iostream>
#include <array>
#include <fstream>
#include <cstring>
#include "oursockets.h"

template <class T>
void print_vector (const std::vector<T>& vec) {
    for (int i = 0; i < vec.size(); i++)
        std::cout << (int) vec[i] << ' ';
    std::cout << std::endl;
}

const int Port = 8888;
const char* SendAddr = "localhost";

int main() {
    MAIN_STARTUP();
    const char* filename = "equippable-items.json";
    std::ifstream f(filename, std::ios::binary | std::ios::ate);
    f.seekg(0, std::ios::beg);
    std::queue<AProtocol::Block> bq;

    // Create UDP socket
    int sockfd;
    // char buffer[AProtocol::BodySize];
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0){
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Fill server address info
    servaddr.sin_family = AF_INET;      // IPv4
    servaddr.sin_port = htons(Port);    // Server Port
    servaddr.sin_addr.s_addr = inet_addr(SendAddr); // Send IP

    socklen_t len = sizeof(servaddr);

    // const char* msg = "Hello!";
    // sendto(sockfd, msg, sizeof(msg), 0, 
    //         (const struct sockaddr*)(&servaddr), len);

    AProtocol::fragment(f, [=](AProtocol::Block b){
        sendto(sockfd, reinterpret_cast<char*>(&b), AProtocol::BlockSize, 0, 
               (const struct sockaddr*)(&servaddr), len);
        if (b.isLast())
            std::cout << "Sent last block" << std::endl;
    });

    close(sockfd);
    // AProtocol::fragment(f, [&bq](AProtocol::Block b){
    //     bq.push(b);
    // });
    f.close();
    std::ofstream of("test-out.json");
    for(;!bq.empty();){
        of.write(bq.front().getBody(), AProtocol::BodySize);
        if (bq.front().isLast())
            std::cout << "Block No. " << bq.front().getCount() << " is last!" << std::endl;
        bq.pop();
    }
    of.close();
    MAIN_CLEANUP();
    return 0;
}