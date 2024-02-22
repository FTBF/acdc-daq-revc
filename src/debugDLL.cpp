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
#include "ACC.h"
#include <getopt.h>
#include <vector>

using namespace std;

int main(int argn, char * argv[])
{
    std::string ip = "192.168.46.107";
    uint32_t vcc_dll = 0x100;
    bool reset = false;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"ip",       required_argument, 0,  'i' },
            {"vdddll",   required_argument, 0,  'v' },
            {"reset",          no_argument, 0,  'r' },
            {0,                          0, 0,   0 }
        };

        int c = getopt_long(argn, argv, "i:v:r", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) 
        {
        case 'i':
            std::cout << optarg << "\t" << strlen(optarg) << std::endl;
            ip = optarg;
            break;

        case 'v':
            vcc_dll = strtol(optarg, NULL, 0);
            break;

        case 'r':
            reset = true;
            break;

        default:
            printf("?? getopt returned character code 0%o ??\n", c);
        }
    }

    ACC acc(ip);
    acc.createAcdcs();

    std::vector<uint32_t> vdd_dll_vector(5, vcc_dll);

    acc.setVddDLL(vdd_dll_vector, reset);

    return 0;
}
