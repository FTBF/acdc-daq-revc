#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <map>
#include <numeric>
#include <vector>
#include "ACC.h"
#include <getopt.h>

int main(int argn, char * argv[])
{
    std::string ip = "192.168.46.107";
    bool set = false;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"ip",       required_argument, 0,  'i' },
            {"set",            no_argument, 0,  's' },
            {0,                          0, 0,   0 }
        };

        int c = getopt_long(argn, argv, "i:s", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) 
        {
        case 'i':
            std::cout << optarg << "\t" << strlen(optarg) << std::endl;
            ip = optarg;
            break;

        case 's':
            set = true;
            break;

        default:
            printf("?? getopt returned character code 0%o ??\n", c);
        }
    }

    ACC acc(ip); 
    
    if(set) acc.scanLinkPhase(0xff, true);
    else    acc.scanLinkPhase(0x00, true);
}
