#pragma once

#include <iostream>

#include <queue>
#include <cstring>
#include <array>
#include <cassert>
#include <cstdint>

// void assert(int pred, std::string message){
//     if (pred == 0)
//         throw std::runtime_error(message);
// }
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

        std::array<char, BodySize> body;
    public:
        Block(unsigned int count, std::array<char, BodySize> data) : 
            block_count(count),
            is_last(false),
            next_packet_offset(0),
            padding(0),
            body(data)
        {};
        Block(unsigned int count, bool is_last, std::array<char, BodySize> data) : 
            block_count(count),
            is_last(is_last),
            next_packet_offset(0),
            padding(0),
            body(data)
        {};
        Block(unsigned int count, unsigned int ofsset, std::array<char, BodySize> data) : 
            block_count(count),
            is_last(false),
            next_packet_offset(ofsset),
            padding(0),
            body(data)
        {};

        bool isLast() const { return is_last; }
        std::array<char, BodySize>& getBody() { return body; }
        unsigned int getCount() const { return block_count; }
        unsigned int getOffset() const { return next_packet_offset; }
        void toggleLast() { is_last = !is_last; }
        void setOffset (unsigned int offset) { next_packet_offset = offset; }
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
    private:
        template<typename T, size_t N>
        static void print_array(const std::array<T, N>& arr){
            for(const auto& e : arr)
                std::cout << (int) e << ' ';
            std::cout << std::endl;
        }
    public:
        static void fragment () {
            if (Packet_Queue.size() == 0) return;
            for (;!Packet_Queue.empty();){
                Packet current_packet = Packet_Queue.front();
                char buff[BodySize] = { 0 };
                unsigned int cursor = 0;
                size_t last_count = 0;
                if (Block_Queue.size() != 0)
                    last_count = Block_Queue.back().getCount();
                if (Block_Queue.size() != 0 && Block_Queue.back().getOffset() != 0) {
                    const unsigned int offset = Block_Queue.back().getOffset();
                    const unsigned int read_size = BodySize - offset;
                    memcpy(buff, &current_packet.getBody().data()[cursor], read_size);
                    std::array<char, BodySize> tmp;
                    std::copy(buff, buff + read_size, std::begin(Block_Queue.back().getBody()) + offset);
                    std::cout << "Changing block: ";
                    print_array<char, BodySize>(tmp);
                    cursor += read_size;
                }
                // std::cout << "From fragment (global): " 
                // << current_packet.size() << ' ' 
                // << current_packet.size() / BodySize << ' '
                // << current_packet.size() % BodySize << ' ';
                // std::cout << cursor << std::endl;
                if (current_packet.size() - cursor >= BodySize){
                    for (unsigned int i = 0; i < (current_packet.size() - cursor) / BodySize; i++){
                        memcpy(buff, &current_packet.getBody().data()[cursor], BodySize);
                        std::array<char, BodySize> tmp;
                        // std::fill(std::begin(tmp), std::end(tmp), 0);
                        memset(tmp.data(), 0, BodySize);
                        std::copy(buff, buff + BodySize, std::begin(tmp));
                        Block_Queue.push(Block(last_count, tmp));
                        std::cout << "Slashing packet (" << (int)tmp[1] << "): ";
                        print_array<char, BodySize>(tmp);
                        // std::cout << "From fragment: " << Block_Queue.front().getCount() << std::endl;
                        cursor += BodySize;
                        last_count++;
                    }
                }
                // Дописать остаток пакета
                if ((current_packet.size() - cursor) % BodySize != 0 || cursor < current_packet.size()) {
                    assert(cursor <= current_packet.size() && "Cursor out of bounds");
                    assert(current_packet.size() - cursor <= BodySize && "Proper entry to if statement"); // TODO придумать сообщение лучше
                    
                    unsigned int offset = current_packet.size() - cursor;
                    memcpy(buff, &current_packet.getBody().data()[cursor], offset);
                    std::array<char, BodySize> tmp;
                    std::fill(std::begin(tmp), std::end(tmp), 0);
                    std::copy(buff, buff + offset, std::begin(tmp));
                    Block_Queue.push(Block(last_count+1, offset, tmp));
                    std::cout << "Adding to block: ";
                    print_array<char, BodySize>(tmp);
                    // std::cout << "From fragment (inside if): " << Block_Queue.back().getOffset() << std::endl;
                }
                Packet_Queue.pop();

                // TODO wait and flag
            };
            Block_Queue.back().toggleLast();
        }
    };
}
