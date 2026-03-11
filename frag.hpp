#pragma once

#include <iostream>

#include <queue>
#include <cstring>
#include <array>
#include <cassert>
#include <cstdint>

namespace AProtocol{
    // static const int BlockSize = 512; // bytes
    static const int BlockSize = 5; 
    static const int HeaderSize = 3;
    static const int BodySize = BlockSize - HeaderSize;
    static const int PacketBufferSize = 4096;
    static_assert(BlockSize > HeaderSize, "Block must be bigger then header");

    class Block {
    private:
        unsigned int block_count     : 8;
        int is_last                  : 1;
        int next_packet_offset       : 9;
        int padding                  : 6;

        const std::array<char, BodySize> body;
    public:
        Block(unsigned int count, std::array<char, BodySize> data) : 
            block_count(0),
            is_last(false),
            next_packet_offset(0),
            padding(0),
            body(data)
        {};
        Block(unsigned int count, bool is_last, std::array<char, BodySize> data) : 
            block_count(0),
            is_last(is_last),
            next_packet_offset(0),
            padding(0),
            body(data)
        {};

        bool isLast() const { return is_last; }
        void toggleLast() { is_last = !is_last; }
        const std::array<char, BodySize>& getBody() const { return body; }
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

    static std::queue<Block> Block_Queue;
    static std::queue<Packet> Packet_Queue;

    class Fragmentation {
    public:
        static void fragment () {
            if (Packet_Queue.size() == 0) return;
            for (; !Packet_Queue.empty(); Packet_Queue.pop()){
                Packet current_packet = Packet_Queue.front();
                char buff[BodySize] = { 0 };
                unsigned int cursor = 0;
                for (unsigned int i = 0; i < current_packet.size() / BodySize; i++){
                    memcpy(buff, &current_packet.getBody().data()[cursor], BodySize);
                    std::array<char, BodySize> tmp;
                    std::copy(buff, buff + BodySize, std::begin(tmp));
                    Block_Queue.push(Block(i, tmp));
                    std::cout << "From fragment: " << current_packet.size() / BodySize << std::endl;
                    cursor += BodySize;
                }
            };
            Block_Queue.front().toggleLast();
        }
    };
}
