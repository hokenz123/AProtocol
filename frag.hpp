#pragma once

#include <iostream>

#include <functional>
#include <fstream>
#include <queue>
#include <cstring>
#include <array>
#include <cassert>
#include <cstdint>

namespace AProtocol{
    static const int BlockSize = 1500; // bytes
    static const int HeaderSize = 4;
    static const int BodySize = BlockSize - HeaderSize;
    // static const int PacketBufferSize = 4096;
    static_assert(BlockSize > HeaderSize, "Block must be bigger then header");

    class Block {
    private:
        unsigned int block_count     : 16;
        int is_last                  : 1;
        int padding                  : 15;

        char body[BodySize];
    public:
        Block(unsigned int count, const char data[BodySize]) : 
            block_count(count),
            is_last(false),
            padding(0)
        {
            memcpy(body, data, BodySize);
        };
        Block(unsigned int count, bool is_last, const char data[BodySize]) : 
            block_count(count),
            is_last(is_last),
            padding(0)
        {
            memcpy(body, data, BodySize);
        };

        bool isLast() const { return is_last; }
        const char* getBody() const { return body; }
        unsigned int getCount() const { return block_count; }
        // void toggleLast() { is_last = !is_last; }
        void setLast(bool value) { is_last = value; }
    };

    class Packet {
    private:
        const size_t packet_size;
        const std::vector<char> body;
    public:
        Packet(size_t size, std::vector<char> data)
        : packet_size(size), body(data) {};
        size_t size() const { return packet_size; }
        const std::vector<char> getBody() const { return body; }
    };

    void print_array(std::array<char, BodySize> arr) {
        for (const auto& e : arr)
            std::cout << e;
        std::cout << std::endl;
    }

    void fragment (std::ifstream& fileStream, std::function<void(Block)> func) {
        char body[BodySize];
        unsigned int block_count = 0;
        while (fileStream)
        {
            fileStream.read(body, BodySize);
            std::streamsize bytesRead = fileStream.gcount();
            if (bytesRead == 0){
                // EOF
                break;
            }
            bool is_last = false;
            if (bytesRead < BodySize){
                memset(body+bytesRead, 0, BodySize-bytesRead);
                is_last = true;
            }
            Block b(block_count, is_last, body);
            if (b.isLast())
                std::cout << "Block No. " << b.getCount() << " is last" << std::endl;
            // blockQueue.push(b);
            func(b);
            block_count++;
        }
    };
}
