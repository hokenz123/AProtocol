#include <iostream>
#include <fstream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <mutex>

static const int PORT = 8888;
static const int BlockSize = 1500;
static const int HeaderSize = 4;
static const int BodySize = BlockSize - HeaderSize;

bool lstEnabled = true;
bool dfgEnabled = true;
std::mutex blocks_mutex;

#pragma pack(push, 1)
struct Block {
    unsigned int block_count : 8;
    int is_last : 1;
    int padding : 23;
    char body[BodySize];
};
#pragma pack(pop)

std::vector<Block> blocks;

void listener(int sockfd, bool &IsEnabled);
void defragmentator_NT(Block block, std::ofstream &out);
void defragmentator(std::vector<Block> &blocks, bool &IsEnabled);

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return 1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Ошибка привязки сокета к порту " << PORT << std::endl;
        close(sockfd);
        return 1;
    }
    
    std::cout << "заходим в прослушивание" << std::endl;
    //listener(sockfd, lstEnabled);

    std::thread lstr(listener, sockfd, std::ref(lstEnabled));
    std::thread defr(defragmentator, std::ref(blocks), std::ref(dfgEnabled));

    std::cout << "Нажмите Enter для остановки..." << std::endl;
    std::cin.get();

    lstEnabled = false;
    dfgEnabled = false;

    defr.join();
    lstr.join();
    std::cout << "выход из программы" << std::endl;
    
    close(sockfd);
    return 0;
}

void listener(int sockfd, bool &IsEnabled) {
    std::cout << "зашли" << std::endl;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BlockSize];

    int packet_count = 0;
    
    //std::ofstream out;
    //out.open("output.txt");

    while (IsEnabled) {
        std::cout << "включен и работает" << std::endl;
        int n = recvfrom(sockfd, buffer, BlockSize, 0,
                        (struct sockaddr*)&client_addr, &client_len);
        
        packet_count++;
        std::cout << "Пакет #" << packet_count << ", размер: " << n << " байт" << std::endl;

        if (n == BlockSize || n == 15 || true) {
            std::cout << "чёто пришло" << std::endl;
            Block block;
            memcpy(&block, buffer, BlockSize);
            {
                std::lock_guard<std::mutex> lock(blocks_mutex);
                blocks.push_back(block);
            }
            std::cout << "Пришёл пакет размером " << n << " байт" << std::endl;
            
            //defragmentator(block, out);
        }
    }
    //out.close();
    
}

void defragmentator_NT(Block block, std::ofstream &out){
    // для непосредственной записи в файл
    
    //std::ofstream out;
    //out.open("output.txt");

    if(out.is_open()){
        out << block.body;
        std::cout << "Записано в файл: " << block.body << std::endl;
        //out.close();
        //lstEnabled = false;
    } else {
        std::cerr << "Ошибка открытия файла" << std::endl;
    }
}

void defragmentator(std::vector<Block> &blocks, bool &IsEnabled){
    //для помещения в поток
    int packNumber;
    
    std::ofstream out;
    out.open("output.txt");
    std::cout << "Файл успешно открыт" << std::endl;
    if(out.is_open()){
        while(IsEnabled){
            if (blocks.size() > 0){
                for(int i = 0; i < blocks.size(); i++){
                    Block curBlock = *blocks.begin();
                    if (curBlock.block_count > packNumber){
                        packNumber = curBlock.block_count;
                        out << curBlock.body;
                        std::cout << "запиано. " << std::endl;
                    }
                }
            }
        }
        out.close();
    } else {
        std::cerr << "Ошибка открытия файла" << std::endl;
    }
}