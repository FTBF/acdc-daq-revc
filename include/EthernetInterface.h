#ifndef ETHERNETINTERFACE_H
#define ETHERNETINTERFACE_H

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <poll.h>
#include <string>
#include <vector>

class EthernetInterface
{
private:
    static constexpr int          MAXBUFLEN_      = 1500;
    static constexpr unsigned int RX_ADDR_OFFSET_ = 2;
    static constexpr unsigned int RX_DATA_OFFSET_ = 10;
    static constexpr unsigned int TX_DATA_OFFSET_ = 2;

    
    int                     sockfd_;
    struct addrinfo         *servinfo, *p_;
    unsigned char           buff_[MAXBUFLEN_];
    fd_set rfds_;
    struct timeval tv_;
    int packetID_;
    
public:

    enum
    {
        DO_NOT_RETURN = 0x02,
        REQUEST_ACK   = 0x04,
        NO_ADDR_INC   = 0x08
    };

    EthernetInterface(std::string ip, std::string port);

    ~EthernetInterface();

    void setBurstTarget();

    void setBurstMode(bool enable);

    void send(uint64_t addr, uint64_t value);

    void send(uint64_t addr, const std::vector<uint64_t>& values);

    uint64_t recieve(uint64_t addr, uint8_t flags = 0);

    std::vector<uint64_t> recieve_many(uint64_t addr, int numwords, uint8_t flags = 0);

    std::vector<uint64_t> recieve_burst(int numwords, int timeout_sec = 65, int timeout_us = 0);
};

#endif
