#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <bitset>
#include <unistd.h>
#include "EthernetInterface.h"

using namespace std;

//this macro is a debugging function
//for sending and listening to data
//transmitted on via the ACC ethernet interface

int main(int argc, char *argv[])
{
    if(argc < 5)
    {
        printf("Requires at least 4 options.  debug [ip] [address] w [value] or debug [ip] [address] r [length] \n");
        exit(0);
    }

    std::string ip = argv[1];

    EthernetInterface eth(ip, "2007");

    uint64_t address = strtol(argv[2],NULL,0);
    uint64_t value = strtol(argv[4],NULL,0);

    //check read or write
    if(argv[3][0] == 'w')
    {
        printf("Sending: %lx, %lx\n", address, value);
        eth.send(address, value);
    }
    else if (argv[3][0] == 'r')
    {
        auto values = eth.recieve_many(address, value);
        for(const auto& val : values)
        {
            printf("read: %8lx: %016lx\n", address, val);
        }
    }
    else
    {
        printf("Option 3 must specify either \"r\" or \"w\" for read or write\n");
        exit(1);
    }
    return 0;
}
