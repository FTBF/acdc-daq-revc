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


int main()
{
    ACC acc; 
    acc.scanLinkPhase(0xff, true);
}
