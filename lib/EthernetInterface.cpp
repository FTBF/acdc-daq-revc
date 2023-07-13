#include "EthernetInterface.h"
#include <iostream>

EthernetInterface::EthernetInterface(std::string ip, std::string port) : sockfd_(-1), servinfo(NULL), packetID_(-1)
{
    struct addrinfo  hints;
    int rv;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if((rv = getaddrinfo(ip.c_str(), port.c_str(), &hints, &servinfo)) != 0)
    {
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    }

    // loop through all the results and make a socket
    for(p_ = servinfo; p_ != NULL; p_ = p_->ai_next)
    {
	if((sockfd_ = socket(p_->ai_family, p_->ai_socktype, p_->ai_protocol)) == -1)
	{
	    perror("sw: socket");
	    continue;
	}

	break;
    }

    if(p_ == NULL)
    {
	fprintf(stderr, "sw: failed to create socket\n");
    }

    FD_ZERO(&rfds_);
    FD_SET(sockfd_, &rfds_);

}

EthernetInterface::~EthernetInterface()
{
    freeaddrinfo(servinfo);
    if(sockfd_ > 0) close(sockfd_);
}

void EthernetInterface::setBurstTarget()
{
    int numbytes;
        
    buff_[0] = 2;                   // set burst reply address
    int packetSz = 1;

    if((numbytes = sendto(sockfd_, buff_, packetSz, 0, p_->ai_addr, p_->ai_addrlen)) ==
       -1)
    {
        perror("sw: sendto");
        exit(1);
    }
}

void EthernetInterface::setBurstMode(bool enable)
{
    int numbytes;
        
    // setup write packet ///////////////////////////////////////////////////////////
    buff_[0] = 1;                   // write
    buff_[1] = 1;                   // num of quadwords
    uint64_t addr    = 0x0000000100000009;  // data enable
    memcpy((void*)&buff_[RX_ADDR_OFFSET_], (void*)&addr, 8);
    memset((void*)&buff_[RX_DATA_OFFSET_], enable?1:0, 1);
    memset((void*)&buff_[RX_DATA_OFFSET_ + 1], 0, 7);  // enable data
    int packetSz = RX_DATA_OFFSET_ + buff_[1] * 8;

    if((numbytes = sendto(sockfd_, buff_, packetSz, 0, p_->ai_addr, p_->ai_addrlen)) ==
       -1)
    {
        perror("sw: sendto");
        exit(1);
    }
}

void EthernetInterface::send(uint64_t addr, uint64_t value)
{
    int numbytes;
    
    buff_[0] = 1;       // write
    buff_[1] = 1;       // num of quadwords
    memcpy((void*)&buff_[RX_ADDR_OFFSET_], (void*)&addr, 8);

    memcpy((void*)&buff_[RX_DATA_OFFSET_],
	   (void*)&value,
	   8);  // increment each time
    int packetSz = RX_DATA_OFFSET_ + buff_[1] * 8;

    if((numbytes = sendto(sockfd_, buff_, packetSz, 0, p_->ai_addr, p_->ai_addrlen)) ==
       -1)
    {
	perror("sw: send");
	exit(1);
    }
}

void EthernetInterface::send(uint64_t addr, const std::vector<uint64_t>& values)
{
    int numbytes;
    
    buff_[0] = 1;       // write
    buff_[1] = values.size();       // num of quadwords
    memcpy((void*)&buff_[RX_ADDR_OFFSET_], (void*)&addr, 8);

    for(int i = 0; i < buff_[1]; ++i)
    {
	memcpy((void*)&buff_[RX_DATA_OFFSET_ + i * 8],
	       (void*)&values[i],
	       8);  // increment each time
    }
    int packetSz = RX_DATA_OFFSET_ + buff_[1] * 8;

    if((numbytes = sendto(sockfd_, buff_, packetSz, 0, p_->ai_addr, p_->ai_addrlen)) ==
       -1)
    {
	perror("sw: send vector");
	exit(1);
    }
}

uint64_t EthernetInterface::recieve(uint64_t addr, uint8_t flags)
{
    int numbytes;
    
    buff_[0] = flags & 0xfe;  // read request
    buff_[1] = 1;  // num words
    memcpy((void*)&buff_[RX_ADDR_OFFSET_], (void*)&addr, 8);
    // ///////////////////////////////////////////////////////////
    int packetSz = RX_DATA_OFFSET_;

    if((numbytes = sendto(sockfd_, buff_, packetSz, 0, p_->ai_addr, p_->ai_addrlen)) ==
       -1)
    {
	perror("sw: sendto");
	exit(1);
    }

    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);

    // read response ///////////////////////////////////////////////////////////
    tv_ = {0, 250000};  // 0 seconds and 250000 useconds
    int retval = select(sockfd_+1, &rfds_, NULL, NULL, &tv_);

    if(retval > 0)
    {
        if((numbytes = recvfrom(sockfd_,
                                buff_,
                                MAXBUFLEN_ - 1,
                                0,
                                (struct sockaddr*)&their_addr,
                                &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        if(packetID_ >= 0 && (packetID_ + 1)%256 != buff_[1]) printf("Missing packet? We jumped from packet id %d to %d\n", packetID_, buff_[1]);
        packetID_ = buff_[1];

    }
    else if(retval == 0)
    {
        printf("Read Timeout\n");
        exit(1);
    }
    else
    {
        perror("select()");
        exit(1);
    }

    uint64_t data;
    memcpy((void*)&data, (void*)&buff_[TX_DATA_OFFSET_], 8);
	
    return data;
}

std::vector<uint64_t> EthernetInterface::recieve_many(uint64_t addr, int numwords, uint8_t flags)
{
    int numbytes;
    
    buff_[0] = (flags & NO_ADDR_INC)?0x08:0x00;  // read request 0 | FLAG BITS
    buff_[1] = (unsigned char)(numwords);  // num words
    memcpy((void*)&buff_[RX_ADDR_OFFSET_], (void*)&addr, 8);
    // ///////////////////////////////////////////////////////////
    int packetSz = RX_DATA_OFFSET_;

    if((numbytes = sendto(sockfd_, buff_, packetSz, 0, p_->ai_addr, p_->ai_addrlen)) ==
       -1)
    {
	perror("sw: sendto");
	exit(1);
    }

    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);

    // read response ///////////////////////////////////////////////////////////
    tv_ = {0, 250000};  // 0 seconds and 250000 useconds
    int retval = select(sockfd_+1, &rfds_, NULL, NULL, &tv_);

    if(retval > 0)
    {
        if((numbytes = recvfrom(sockfd_,
                                buff_,
                                MAXBUFLEN_ - 1,
                                0,
                                (struct sockaddr*)&their_addr,
                                &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        if(packetID_ >= 0 && (packetID_ + 1)%256 != buff_[1]) printf("Missing packet? We jumped from packet id %d to %d\n", packetID_, buff_[1]);
        packetID_ = buff_[1];
    }
    else if(retval == 0)
    {
        printf("Read many Timeout\n");
        exit(1);
    }
    else
    {
        perror("select()");
        exit(1);
    }

    std::vector<uint64_t> data((numbytes-2)/8);
    memcpy((void*)data.data(), (void*)&buff_[TX_DATA_OFFSET_], numbytes - 2);
	
    return data;
}

std::vector<uint64_t> EthernetInterface::recieve_burst(int numwords, int timeout_sec, int timeout_us )
{
    int numbytes;
    
    std::vector<uint64_t> data(numwords);
    
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);

    int wordsRead = 0;
    buff_[0] = 0;
    
    while(wordsRead < numwords)
    {
        // read response ///////////////////////////////////////////////////////////
        tv_ = {timeout_sec, timeout_us};  // 0 seconds and 250000 useconds
        int retval = select(sockfd_+1, &rfds_, NULL, NULL, &tv_);
        if(retval > 0)
        {
            if((numbytes = recvfrom(sockfd_,
                                    buff_,
                                    MAXBUFLEN_ - 1,
                                    0,
                                    (struct sockaddr*)&their_addr,
                                    &addr_len)) == -1)
            {
                perror("recvfrom");
                exit(1);
            }
            if(!((buff_[0] & 0x7) == 1 || (buff_[0] & 0x7) == 2 || (buff_[0] & 0x7) == 3)) printf("Not burst packet! %x\n", buff_[0]); 

            if(packetID_ >= 0 && (packetID_ + 1)%256 != buff_[1]) printf("Missing burst packet? We jumped from packet id %d to %d\n", packetID_, buff_[1]);
            packetID_ = buff_[1];

            for (int i = 0; i < (numbytes-2)/8; ++i)
            {
                if(i+wordsRead < numwords)
                {
                    memcpy((void*)(data.data()+i+wordsRead), (void*)&buff_[TX_DATA_OFFSET_ + 8*i], 8);
                }
                else
                {
                    break;
                }
            }
        
            wordsRead += (numbytes-2)/8;
        }
        else if(retval == 0)
        {
            printf("Burst Read Timeout\n");
            exit(1);
        }
        else
        {
            perror("select()");
            exit(1);
        }
    }

    return data;
}
