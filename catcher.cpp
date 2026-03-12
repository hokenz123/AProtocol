#include <iostream>
#include <queue>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

std::queue<std::vector<uint8_t>> packet_queue;
std::mutex queue_mutex;

void savePacket(const uint8_t* data, int size) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    packet_queue.push(std::vector<uint8_t>(data, data + size));
    std::cout << " [+] Пинг перехвачен! В очереди: " << packet_queue.size() << "\n";
}

int packetCallback(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
                   struct nfq_data *nfa, void *data) {
    
    uint8_t *pkt_data;
    int len = nfq_get_payload(nfa, &pkt_data);
    
    if (len > 0) {
        struct iphdr *ip = (struct iphdr*)pkt_data;
        
        if (ip->protocol == 1) {
            int ip_len = ip->ihl * 4;
            struct icmphdr *icmp = (struct icmphdr*)(pkt_data + ip_len);
            
            if (icmp->type == 8) {
                savePacket(pkt_data, len);
                
                uint32_t packet_id = 0;
                struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(nfa);
                if (ph) {
                    packet_id = ntohl(ph->packet_id);
                }
                
                return nfq_set_verdict(qh, packet_id, NF_DROP, 0, NULL);
            }
        }
    }
    
    return nfq_set_verdict(qh, 0, NF_ACCEPT, 0, NULL);
}

int main() {
    std::cout << "\n=== БЛОКИРОВЩИК ПИНГОВ ===\n";
    std::cout << "Команды: g - показать пакеты, q - выход\n\n";
    
    struct nfq_handle *h = nfq_open();
    if (!h) {
        std::cerr << "Ошибка nfq_open\n";
        return 1;
    }
    
    nfq_unbind_pf(h, AF_INET);
    if (nfq_bind_pf(h, AF_INET) < 0) {
        std::cerr << "Ошибка bind\n";
        nfq_close(h);
        return 1;
    }
    
    struct nfq_q_handle *qh = nfq_create_queue(h, 0, &packetCallback, NULL);
    if (!qh) {
        std::cerr << "Ошибка создания очереди\n";
        nfq_close(h);
        return 1;
    }
    
    nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xFFFF);
    int fd = nfq_fd(h);
    
    std::cout << "ГОТОВО! Пинги блокируются.\n\n";
    
    char buf[4096];
    fd_set fds;
    
    while (true) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        FD_SET(0, &fds);
        
        select(fd + 1, &fds, NULL, NULL, NULL);
        
        if (FD_ISSET(0, &fds)) {
            char cmd;
            read(0, &cmd, 1);
            
            if (cmd == 'q') break;
            if (cmd == 'g') {
                std::lock_guard<std::mutex> lock(queue_mutex);
                std::cout << "\nПакетов в очереди: " << packet_queue.size() << "\n";
                while (!packet_queue.empty()) {
                    std::cout << "  Пакет размером " << packet_queue.front().size() << " байт\n";
                    packet_queue.pop();
                }
                std::cout << "\n";
            }
        }
        
        if (FD_ISSET(fd, &fds)) {
            int rv = recv(fd, buf, sizeof(buf), 0);
            if (rv > 0) nfq_handle_packet(h, buf, rv);
        }
    }
    
    nfq_destroy_queue(qh);
    nfq_close(h);
    return 0;
}