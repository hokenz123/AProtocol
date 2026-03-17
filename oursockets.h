#include <unistd.h>
#if defined(_WIN32) || defined(_WIN64)
    #define __WIN
#endif
#define MAIN_STARTUP()
#define MAIN_CLEANUP()
#ifdef __WIN
    #include <winsock2.h>
    #include <ws2tcpip.h> // Для inet_pton, addrinfo
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
    #undef MAIN_STARTUP
    #define MAIN_STARTUP()          \
        WSADATA wsdata;             \
        WSAStartup(0x0101,&wsdata)
    #undef MAIN_CLEANEUP
    #define MAIN_CLEANEUP() WSACleanup()
#else
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif