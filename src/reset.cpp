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
    ACC acc;
    acc.resetACC();
    usleep(5000);
    acc.resetACDC();

    return 0;
}
