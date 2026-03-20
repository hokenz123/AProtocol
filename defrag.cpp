#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include "oursockets.h"
#include <thread>
#include <mutex>
#include <chrono>
#include <iomanip>

static const int PORT = 8888;
static const int BlockSize = 1500;
static const int HeaderSize = 4;
static const int BodySize = BlockSize - HeaderSize;

bool lstEnabled = true;
bool dfgEnabled = true;
bool monEnabled = true;
std::mutex swap_mutex;

long long total_packets = 0;
long long total_bytes = 0;

#pragma pack(push, 1)
struct Block {
    unsigned int block_count : 16;
    int is_last : 1;
    int padding : 15;
    char body[BodySize];
};
#pragma pack(pop)

std::vector<Block> blocksA;
std::vector<Block> blocksB;
std::vector<Block> *buffs[2] = { &blocksA, &blocksB }; 

namespace AProtocol {
    void listener(int sockfd, std::vector<Block> *BBuf[], bool &IsEnabled);
    void defragmentator(std::vector<Block> *blocks[], bool &IsEnabled);
    void monitor(bool &IsEnabled);
}

int main() {
    MAIN_STARTUP();
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket to port" << PORT << std::endl;
        close(sockfd);
        return 1;
    }
    
    std::cout << "Entering listening" << std::endl;

    std::thread lstr(AProtocol::listener, sockfd, buffs, std::ref(lstEnabled));
    std::thread defr(AProtocol::defragmentator, buffs, std::ref(dfgEnabled));
    std::thread mon(AProtocol::monitor, std::ref(monEnabled));

    std::cout << "Нажмите Enter для остановки..." << std::endl;
    std::cin.get();

    lstEnabled = false;
    dfgEnabled = false;
    monEnabled = false;

    defr.join();
    lstr.join();
    mon.join();

    std::cout << "Exit out of program" << std::endl;
    
    close(sockfd);
    MAIN_CLEANEUP();
    return 0;
}

namespace AProtocol{

    void listener(int sockfd,std::vector<Block> *BBuf[], bool &IsEnabled) { //315 pps avg 

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        char buffer[BlockSize];

        total_packets = 0;
        total_bytes = 0;

        BBuf[0]->reserve(2000);
        
        while (IsEnabled) {

            BBuf[0]->emplace_back();
            Block& block = BBuf[0]->back();

            int n = recvfrom(sockfd, &block, BlockSize, 0,
                            (struct sockaddr*)&client_addr, &client_len);
            
            if(n > 100){

                total_packets ++ ;
                total_bytes += n ;

                //memcpy(&block, buffer, n);
                //BBuf[0]->push_back(std::move(block));
                
                {
                    if (total_packets % 1000 == 0 || block.is_last){
                        std::lock_guard<std::mutex> lock(swap_mutex);
                        std::swap(BBuf[0],BBuf[1]);
                    }
                }

            } else {
                BBuf[0]->pop_back();
                continue;
            }
            
        }
        
    }

    void defragmentator(std::vector<Block> *blocks[], bool &IsEnabled){
        int packNumber = 0;
        
        std::ofstream out;
        out.open("recieved.txt");
        std::cout << "File opened successfully" << std::endl;
        if(out.is_open()){
            while(IsEnabled){

                std::vector<Block> blocksToWrite;

                {
                    if (!blocks[1]->empty()) {
                        std::lock_guard<std::mutex> lock(swap_mutex);
                        blocksToWrite.swap(*blocks[1]);          
                    }
                }   

                for (const auto& block : blocksToWrite) {
                    out.write(block.body, BodySize);
                    out.flush();
                    //std::cout << "Wrote block #" << (int)block.block_count << std::endl;
                }

            }
            out.close();
        } else {
            std::cerr << "Error opening file" << std::endl;
        }
    }

    void monitor(bool &IsEnabled){

        using namespace std::chrono;

        time_point last_time = steady_clock::now();
        long long last_packets = 0;
        long long last_bytes = 0;

        while (IsEnabled){
            std::this_thread::sleep_for(milliseconds(100));
        
            time_point current_time = steady_clock::now();
            int64_t ns = duration_cast<nanoseconds>(current_time - last_time).count();

            if (ns >= 1'000'000'000) {
                long long packets_diff = total_packets - last_packets;
                long long bytes_diff = total_bytes - last_bytes;
                
                double speed_gbps = (bytes_diff * 8.0) / ns;

                //TODO:replace this shit with qt
                std::cout << "\n=== SPEED STATS ===" << std::endl;
                std::cout << "Time interval: " << ns << " ns (" << ns / 1'000'000'000.0 << " sec)" << std::endl;
                std::cout << "Packets: " << packets_diff << " pps" << std::endl;
                std::cout << "Data: " << bytes_diff << " bytes (" << bytes_diff / (1024.0 * 1024.0) << " MB)" << std::endl;
                std::cout << "Speed: " << std::fixed << std::setprecision(3) << speed_gbps << " Gbps" << std::endl;
                std::cout << "Total packets: " << total_packets << std::endl;
                std::cout << "Total data: " << (total_bytes / (1024.0 * 1024.0 * 1024.0)) << " GB" << std::endl;
                std::cout << "==================" << std::endl;

                
                last_packets = total_packets;
                last_bytes = total_bytes;
                last_time = current_time;
            }
        }

    }
}