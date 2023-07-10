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

using namespace std;

int main()
{
    ACC acc("192.168.46.109");
    acc.configJCPLL();
    
    return 0;
}
