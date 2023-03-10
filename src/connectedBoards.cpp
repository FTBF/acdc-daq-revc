#include "ACC.h"
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <bitset>
#include <unistd.h>
#include <getopt.h>

using namespace std;

int main(int argn, char * argv[])
{
    std::string ip = "192.168.46.107";
    bool verbose = false;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"ip",       required_argument, 0,  'i' },
            {"verbose",        no_argument, 0,  'v' },
            {0,                          0, 0,   0 }
        };

        int c = getopt_long(argn, argv, "i:v", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) 
        {
        case 'i':
            std::cout << optarg << "\t" << strlen(optarg) << std::endl;
            ip = optarg;
            break;

        case 'v':
            verbose = true;
            break;

        default:
            printf("?? getopt returned character code 0%o ??\n", c);
        }
    }

    ACC acc(ip);

    if(verbose) acc.versionCheck(true);
    else        acc.versionCheck(false);
}
