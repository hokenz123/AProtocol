#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include "oursockets.h"
//#include <thread>

static const int PORT = 8888;
static const int BlockSize = 1500;
static const int HeaderSize = 4;
static const int BodySize = BlockSize - HeaderSize;

bool lstEnabled = true;
bool dfgEnabled = true;

#pragma pack(push, 1)
struct Block {
    unsigned int block_count : 8;
    int is_last : 1;
    int padding : 23;
    char body[BodySize];
};
#pragma pack(pop)

std::vector<Block> blocks;

void listener(int sockfd);
void defragmentator(Block block);
void defragmentator(std::vector<Block> &blocks, bool &IsEnabled);

int main() {
    MAIN_STARTUP();
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        // std::cerr << "Ошибка создания сокета" << std::endl;
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        // std::cerr << "Ошибка привязки сокета к порту " << PORT << std::endl;
        std::cerr << "Error binding socket to port" << PORT << std::endl;
        close(sockfd);
        return 1;
    }
    
    // std::cout << "заходим в прослушивание" << std::endl;
    std::cout << "Entering listening" << std::endl;
    listener(sockfd);

    // std::cout << "выход из программы" << std::endl;
    std::cout << "Exit out of program" << std::endl;
    // std::thread lstr(listener, {sockfd, lstEnabled});
    // std::thread defr(defragmentator, {blocks, dfgEnabled});
    //TODO: Потоки не работают
    // char a;
    // std::cin >> a;

    // lstEnabled = false;
    // dfgEnabled = false;

    // defr.join();
    // lstr.join();
    
    close(sockfd);
    MAIN_CLEANUP();
    return 0;
}

void listener(int sockfd) {
    // std::cout << "зашли" << std::endl;
    std::cout << "Entered" << std::endl;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BlockSize];
    
    while (lstEnabled) {
        // std::cout << "включен и работает" << std::endl;
        std::cout << "Enabled and working (" << blocks.size() << ')' << std::endl;
        int n = recvfrom(sockfd, buffer, BlockSize, 0,
                        (struct sockaddr*)&client_addr, &client_len);
        
        if (n == BlockSize) {
            //std::cout << "чёто пришло" << std::endl;
            std::cout << "Something recieved" << std::endl;
            Block block;
            memcpy(&block, buffer, BlockSize);
            blocks.push_back(block);
            //std::cout << "Пришёл пакет размером " << n << " байт" << std::endl;
            std::cout << "Got packet of size " << n << " byte" << std::endl;
            defragmentator(block);
        }
    }
}

void defragmentator(Block block){
    // для непосредственной записи в файл
    
    std::ofstream out;
    out.open("output.txt", std::ios::app);

    if(out.is_open()){
        out << block.body;
        //std::cout << "Записано в файл: " << block.body << std::endl;
        // std::cout << "Wrote to file: " << block.body << std::endl;
        out.close();
        if (block.is_last)
            lstEnabled = false;
    } else {
        //std::cerr << "Ошибка открытия файла" << std::endl;
        std::cerr << "Error opening file" << std::endl;
    }
}

void defragmentator(std::vector<Block> &blocks, bool &IsEnabled){
    //для помещения в поток
    int packNumber;
    
    std::ofstream out;
    out.open("output.txt", std::ios::app);
    //std::cout << "Файл успешно открыт" << std::endl;
    std::cout << "File opened successfully" << std::endl;
    if(out.is_open()){
        while(IsEnabled){
            if (blocks.size() > 0){
                for(int i = 0; i < blocks.size(); i++){
                    Block curBlock = *blocks.begin();
                    if (curBlock.block_count > packNumber){
                        packNumber = curBlock.block_count;
                        out << curBlock.body;
                    }
                }
            }
        }
        out.close();
    } else {
        //std::cerr << "Ошибка открытия файла" << std::endl;
        std::cerr << "Error opening file" << std::endl;
    }
}