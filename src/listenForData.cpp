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
    YAML::Node config = YAML::LoadFile("test.yaml");

    YAML::Node config_DAQ = config["basicConfig"]["DAQSettings"];

    int retval;

    int triggermode;

    unsigned int boardmask;

    int calibMode;

    int rawMode;

    int invertMode;

    int eventCounter;
    int eventNumber;
    int failCounter;
	
    int threshold;

    string timestamp;
    string label;

    int run=0;

    float validationStart;
    float validationWindow;

    int psec_chip;
    unsigned int psec_channel;
	
    int ACCsw;
    int ACDCsw;
	
    std::vector<unsigned int> vec_psec_channel = {0x3f, 0x3f, 0x3f, 0x3f, 0x3f};

    int BeamgateMultiplexer;
    int PPS_divide_ratio;
	
    int metaSwitch;

    ACC acc(config_DAQ["ip"].as<std::string>());

    system("mkdir -p Results");

    boardmask = config_DAQ["ACDCMask"].as<unsigned int>();
	
    if(config_DAQ["fileLabel"]) label = config_DAQ["fileLabel"].as<std::string>();

    if(config_DAQ["resetACCOnStart"]  && config_DAQ["resetACCOnStart"].as<bool>())  acc.resetACC();
    if(config_DAQ["resetACDCOnStart"] && config_DAQ["resetACDCOnStart"].as<bool>()) acc.resetACDC();

    if(config_DAQ["pedestals"])
    {
        if(config_DAQ["pedestals"].IsScalar())
        {
                std::cout << "PEDS SET" << std::endl;
            acc.setPedestals(boardmask, 0x1f, config_DAQ["pedestals"].as<int>());
        }
        else if(config_DAQ["pedestals"].IsSequence())
        {
            const auto& peds = config_DAQ["pedestals"].as<std::vector<int>>();
            if(peds.size() != 5)
            {
                acc.writeErrorLog("Incorrect pedestal configuration");
            }
            for(unsigned int iPed = 0; iPed < peds.size(); ++iPed)
            {
                acc.setPedestals(boardmask, 0x1 << iPed, peds[iPed]);
            }
        }
    }

    triggermode = config_DAQ["triggerMode"].as<int>();

    if(triggermode!=1)
    {
        switch(triggermode)
        {
                    
        case 2:
        case 3:

            if(config_DAQ["accTrigPolarity"]) acc.setSign(config_DAQ["accTrigPolarity"].as<int>(), 2);
            if(triggermode == 2) goto selfsetup;

            if(config_DAQ["validationStart"])  acc.setValidationStart(config_DAQ["validationStart"].as<int>());
            if(config_DAQ["validationWindow"]) acc.setValidationWindow(config_DAQ["validationWindow"].as<int>());

            goto selfsetup;
        case 4:
            //hi there
            break;
                                
        default:
            std::cout << " Trigger input not found " << std::endl;
            break;
        selfsetup:
            if(config_DAQ["selfTrigPolarity"]) acc.setSign(config_DAQ["selfTrigPolarity"].as<int>(), 4);

            if(config_DAQ["selfTrigThresholds"])
            {
                if(config_DAQ["selfTrigThresholds"].IsScalar())
                {
                    std::cout << "THRESHOLD SET" << std::endl;
                    acc.setThresholds(std::vector<unsigned int>(30, config_DAQ["selfTrigThresholds"].as<unsigned int>()));
                }
                else if(config_DAQ["selfTrigThresholds"].IsSequence())
                {
                    acc.setThresholds(config_DAQ["selfTrigThresholds"].as<std::vector<unsigned int>>());
                }
            }

            if(config_DAQ["selfTrigMask"]) acc.setPsecChannelMask(config_DAQ["selfTrigMask"].as<std::vector<unsigned int>>());
            else                           acc.setPsecChannelMask(vec_psec_channel);

            acc.setEnableCoin(0);

        }
    }
		
    calibMode = config_DAQ["calibMode"].as<bool>();
    rawMode   = !config_DAQ["humanReadableData"].as<bool>();

    eventNumber = config_DAQ["nevents"].as<int>();

    retval = acc.initializeForDataReadout(triggermode, boardmask, calibMode);
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
        if(triggermode == 1 || triggermode == 4)
        {
            acc.softwareTrigger();
        }
        if(eventCounter>=reTime*mult)
        {
            timestamp = getTime();
            mult++;
        }
        retval = acc.listenForAcdcData(triggermode, rawMode, timestamp, label);
        switch(retval)
        {
        case 0:
            //std::cout << "Successfully found data and parsed" << std::endl;
            eventCounter++;
            failCounter=0;
            break;
        case 1:
            writeErrorLog("Successfully found data and but buffer corrupted");
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
	
    auto t1 = std::chrono::high_resolution_clock::now();
    auto dt = 1.e-9*std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
    cout << "It took "<< dt <<" second(s)."<< endl;
    return 1;
}


