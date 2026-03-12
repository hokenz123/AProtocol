#include <iostream>
#include <queue>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>     

std::queue<std::vector<uint8_t>> packet_queue; 

void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet);
void addPacketToQueue(const uint8_t* data, int size);

int main() {

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap *handler;

    // 1. Открыть устройство для захвата
    handler = pcap_open_live("any",65535,1,1000,errbuf);

    if (handler == nullptr) {
        std::cerr << "Ошибка: " << errbuf << std::endl;
        return 1;
    }
    
    std::cout << "Устройство открыто успешно!" << std::endl;
    // 2. Установить фильтр

    struct bpf_program fp;
    char filter_exp[] = "icmp";

    if (pcap_compile(handler, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        std::cerr << "Ошибка компиляции фильтра" << std::endl;
        return 1;
    }

    if (pcap_setfilter(handler, &fp) == -1){
        std::cerr << "Ошибка установки фильтра" << std::endl;
        return 1;
    }
    std::cout << "Фильтр установлен успешно!" << std::endl;
    // 3.  Запустить цикл захвата
    pcap_loop(handler,0,packet_handler,nullptr);
    // 4. Закрыть устройство
    pcap_close(handler);
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
    std::lock_guard<std::mutex> lock(queue_mutex);  // блокируем доступ для других потоков
    std::vector<uint8_t> packet(data, data + size); // копируем данные пакета
    packet_queue.push(packet);                       // кладём в очередь
    std::cout << " [+] Пинг перехвачен! Пакетов в очереди: " << packet_queue.size() << std::endl;
}