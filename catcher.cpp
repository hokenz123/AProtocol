#include <iostream>
#include <queue>
#include <mutex>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>     

std::queue<std::vector<uint8_t>> packet_queue; 
std::mutex queue_mutex; 

void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet);
void addPacketToQueue(const uint8_t* data, int size);

int main() {
    std::cout << "=== ПЕРЕХВАТЧИК PING ПАКЕТОВ ===" << std::endl;
    std::cout << "Все ping запросы будут перехвачены и заблокированы!" << std::endl;
    std::cout << "Пакеты складываются в очередь для обработки" << std::endl;
    std::cout << "Нажмите:\n";
    std::cout << "  'g' - получить все пакеты из очереди\n";
    std::cout << "  'q' - выход\n\n";
    
    // Шаг 1: Открываем соединение с netfilter
    struct nfq_handle *netfilter_handle = nfq_open();
    if (!netfilter_handle) {
        std::cerr << "ОШИБКА: Не удалось открыть netfilter" << std::endl;
        return 1;
    }
    
    // Шаг 2: Отключаем предыдущие обработчики и подключаемся к IPv4
    nfq_unbind_pf(netfilter_handle, AF_INET);
    if (nfq_bind_pf(netfilter_handle, AF_INET) < 0) {
        std::cerr << "ОШИБКА: Не удалось привязаться к IPv4" << std::endl;
        return 1;
    }
    
    // Шаг 3: Создаём очередь для пакетов (номер 0)
    struct nfq_q_handle *queue_handle = nfq_create_queue(netfilter_handle, 0, &packetCallback, nullptr);
    if (!queue_handle) {
        std::cerr << "ОШИБКА: Не удалось создать очередь" << std::endl;
        return 1;
    }
    
    // Шаг 4: Настраиваем режим захвата (копируем весь пакет)
    if (nfq_set_mode(queue_handle, NFQNL_COPY_PACKET, 0xFFFF) < 0) {
        std::cerr << "ОШИБКА: Не удалось установить режим захвата" << std::endl;
        return 1;
    }
    
    // Шаг 5: Получаем файловый дескриптор для чтения пакетов
    int file_descriptor = nfq_fd(netfilter_handle);
    
    std::cout << "ГОТОВО! Теперь все ping запросы будут перехватываться." << std::endl;
    std::cout << "Попробуйте пинговать - пинг НЕ будет работать!\n" << std::endl;
    
    // Шаг 6: Основной цикл обработки
    char buffer[4096];
    fd_set read_fds;
    
    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(file_descriptor, &read_fds);  
        FD_SET(0, &read_fds);                 
        
        select(file_descriptor + 1, &read_fds, nullptr, nullptr, nullptr);

        if (FD_ISSET(0, &read_fds)) {
            char command;
            read(0, &command, 1);
            
            if (command == 'g') {
                
                std::cout << "\n--- ПОЛУЧАЕМ ПАКЕТЫ ИЗ ОЧЕРЕДИ ---" << std::endl;
                auto packets = getAllPackets();
                
                
                std::cout << "Получено " << packets.size() << " пакетов" << std::endl;
                
                
                for (size_t i = 0; i < packets.size() && i < 3; i++) {
                    std::cout << "  Пакет " << i+1 << ": " << packets[i].size() << " байт" << std::endl;
                }
                std::cout << std::endl;
            }
            else if (command == 'q') {
                std::cout << "Выход..." << std::endl;
                break;
            }
        }
        
        if (FD_ISSET(file_descriptor, &read_fds)) {
            int bytes_read = recv(file_descriptor, buffer, sizeof(buffer), 0);
            if (bytes_read > 0) {
                nfq_handle_packet(netfilter_handle, buffer, bytes_read);
            }
        }
    }
    
    nfq_destroy_queue(queue_handle);
    nfq_close(netfilter_handle);
    
    return 0;
}

void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet){
    std::cout << "Получен пакет! Размер: " << pkthdr->len << " байт" << std::endl;

    // Всегда отрезаем 16 байт (для "any" интерфейса)
    const u_char *ip_packet = packet + 16;
    int ip_len = pkthdr->caplen - 16;
    
    std::cout << "IP пакет: " << ip_len << " байт" << std::endl;
    std::cout << "Первые байты: ";
    for (int i = 0; i < 20 && i < ip_len; i++) {
        printf("%02x ", ip_packet[i]);
    }
    std::cout << std::endl;
}

void addPacketToQueue(const uint8_t* data, int size) {
    std::lock_guard<std::mutex> lock(queue_mutex);  
    std::vector<uint8_t> packet(data, data + size); 
    packet_queue.push(packet);                      
    std::cout << " [+] Пинг перехвачен! Пакетов в очереди: " << packet_queue.size() << std::endl;
}

std::vector<std::vector<uint8_t>> getAllPackets() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    
    std::vector<std::vector<uint8_t>> result;
    
    while (!packet_queue.empty()) {
        result.push_back(packet_queue.front());
        packet_queue.pop();
    }
    
    std::cout << " [*] Взято пакетов из очереди: " << result.size() << std::endl;
    return result;
}