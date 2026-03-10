
namespace AProtocol{
    class Block {
        int block_count : 8;
        int is_last : 1;
        int next_packet_offset : 9;
        const char* body;
    }
}
