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


int main(int argn, char ** argv)
{
    ACC acc; 
    
    if(argn > 1) acc.scanLinkPhase(strtoul(argv[1], nullptr, 0), true);
    else         acc.scanLinkPhase(0x00, true);
}
