#include "ACC.h"
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <bitset>
#include <unistd.h>
#include <limits>
#include <chrono> 
#include <iomanip>
#include <numeric>
#include <ctime>
#include <vector>
#include <stdio.h>

string getTime()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    char timeVal[256];
    strftime(timeVal, 256, "%Y%m%d_%H%M%S", std::localtime(&in_time_t));
    return std::string(timeVal);
}

void writeErrorLog(string errorMsg)
{
    std::string err = "errorlog.txt";
    std::cout << "------------------------------------------------------------" << std::endl;
   	std::cout << errorMsg << endl;
    std::cout << "------------------------------------------------------------" << std::endl;
    ofstream os_err(err, ios_base::app);
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
//    std::stringstream ss;
//    ss << std::put_time(std::localtime(&in_time_t), "%m-%d-%Y %X");
//    os_err << "------------------------------------------------------------" << std::endl;
//    os_err << ss.str() << endl;
//    os_err << errorMsg << endl;
//    os_err << "------------------------------------------------------------" << std::endl;
//    os_err.close();
}

int main()
{
    //YAML::Node config = YAML::LoadFile("calibPed.yaml");
    YAML::Node config = YAML::LoadFile("test.yaml");

    YAML::Node config_DAQ = config["basicConfig"]["DAQSettings"];

    int retval;

    int eventCounter;
    int eventNumber;
    int failCounter;
	
    int threshold;

    string timestamp;

    ACC acc(config_DAQ["ip"].as<std::string>());

    eventNumber = config_DAQ["nevents"].as<int>();

    system("mkdir -p Results");

    retval = acc.initializeForDataReadout(config_DAQ);
    if(retval != 0)
    {
        cout << "Initialization failed!" << endl;
        return 0;
    }
    acc.dumpData(0xFF);
    timestamp = getTime();

    eventCounter = 0;
    failCounter = 0;
    int reTime = 500;
    int mult = 1;
    auto t0 = std::chrono::high_resolution_clock::now();

    while(eventCounter<eventNumber)
    {
        if(acc.params_.triggerMode == 1)
        {
            acc.softwareTrigger();
        }
        if(eventCounter>=reTime*mult)
        {
            //timestamp = getTime();
            mult++;
        }
        retval = acc.listenForAcdcData(timestamp);
        switch(retval)
        {
        case 0:
            //std::cout << "Successfully found data and parsed" << std::endl;
            eventCounter++;
            failCounter=0;
            break;
        case 1:
            writeErrorLog("Successfully found data but buffer corrupted");
            acc.dumpData(0xFF);
            failCounter++;
            break;
        case 2:
            writeErrorLog("No data found");
            acc.dumpData(0xFF);
            failCounter++;
            break;
        case 3:
            writeErrorLog("Sigint failure");
            acc.dumpData(0xFF);
            failCounter=50;
            break;
        default:
            writeErrorLog("Unknown error");
            failCounter++;
            break;
        }
        if(failCounter >= 50)
        {
            std::cout << "Too many failed attempts to read data. Please check everything and try again" << std::endl;
            break;
        }
    }

    acc.endRun();
	
    auto t1 = std::chrono::high_resolution_clock::now();
    auto dt = 1.e-9*std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
    cout << "It took "<< dt <<" second(s)."<< endl;
    return 1;
}


