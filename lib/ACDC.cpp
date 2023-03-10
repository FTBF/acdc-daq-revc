#include "ACDC.h"
#include <bitset>
#include <sstream>
#include <fstream>
#include <chrono> 
#include <iomanip>
#include <numeric>
#include <ctime>

using namespace std;

ACDC::ACDC() : boardIndex(-1), outFile_(nullptr), nEvents_(0) {}

ACDC::ACDC(int bi) : boardIndex(bi), outFile_(nullptr), nEvents_(0) {}

ACDC::~ACDC()
{
    if(outFile_)
    {
        outFile_->close();
    }
}

ACDC::ConfigParams::ConfigParams() :
    reset(false),
    pedestals(0x800, 5),
    selfTrigPolarity(0),
    triggerThresholds(0x780, 30),
    selfTrigMask(0x3f, 5),
    calibMode(false),
    dll_vdd(0xcff),
    acc_backpressure(true)
{
}

int ACDC::getBoardIndex() const
{
	return boardIndex;
}

void ACDC::setBoardIndex(int bi)
{
	boardIndex = bi;
}

void ACDC::parseConfig(const YAML::Node& config)
{
    if(config["resetACDCOnStart"]) params_.reset = config["resetACDCOnStart"].as<bool>();
    if(config["pedestals"])
    {
        if(config["pedestals"].IsScalar())
        {
            params_.pedestals = std::vector<unsigned int>(5, config["pedestals"].as<unsigned int>());
        }
        else if(config["pedestals"].IsSequence())
        {
            params_.pedestals = config["pedestals"].as<std::vector<unsigned int>>();
            if(params_.pedestals.size() != 5)
            {
                //acc.writeErrorLog("Incorrect pedestal configuration");
            }
        }
    }
    if(config["selfTrigPolarity"]) params_.selfTrigPolarity = config["selfTrigPolarity"].as<int>();
    if(config["selfTrigThresholds"])
    {
        if(config["selfTrigThresholds"].IsScalar())
        {
            params_.triggerThresholds = std::vector<unsigned int>(30, config["selfTrigThresholds"].as<unsigned int>());
        }
        else if(config["selfTrigThresholds"].IsSequence())
        {
            params_.triggerThresholds = config["selfTrigThresholds"].as<std::vector<unsigned int>>();
        }
    }
    if(config["selfTrigMask"]) params_.selfTrigMask = config["selfTrigMask"].as<std::vector<unsigned int>>();
    if(config["calibMode"]) params_.calibMode = config["calibMode"].as<bool>();
    if(config["accBackpressure"]) params_.acc_backpressure = config["accBackpressure"].as<bool>();
    if(config["dll_vdd"]) params_.dll_vdd = config["dll_vdd"].as<unsigned int>();
}

//looks at the last ACDC buffer and organizes
//all of the data into a data map. The boolean
//argument toggles whether you want to subtract
//pedestals and convert ADC-counts to mV live
//or keep the data in units of raw ADC counts. 
//retval:  ... 1 and 2 are never returned ... 
//2: other error
//1: corrupt buffer 
//0: all good
int ACDC::parseDataFromBuffer(const vector<uint64_t>& buffer)
{
    //Catch empty buffers
    if(buffer.size() == 0)
    {
        std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
        return -1;
    }

    //clear the data map prior.
    data.clear();

    //check for fixed words in header
    if(((buffer[1] >> 48) & 0xffff) != 0xac9c || (buffer[4] & 0xffff) != 0xcac9)
    {
        printf("%lx, %lx\n", (buffer[1] >> 48) & 0xffff, buffer[4] & 0xffff);
        std::cout << "Data buffer header corrupt" << std::endl;
        return -2;
    }

    //Fill data map
    int channel_count = 0;
    int cap_count = 0;
    decltype(data.emplace()) empl_retval;
    for(unsigned int i = 5; i < buffer.size(); ++i)
    {
        for(int j = 4; j >=0; --j)
        {
            if(cap_count == 0) empl_retval = data.emplace(std::piecewise_construct, std::forward_as_tuple(channel_count), std::forward_as_tuple(256));
            (*(empl_retval.first)).second[cap_count] = (buffer[i] >> (j*12)) & 0xfff;
            ++cap_count;
            if(cap_count >= 256)
            {
                cap_count = 0;
                ++channel_count;
            }
        }
    }

    if(data.size()!=NUM_CH)
    {
        cout << "error 1: Not 30 channels " << data.size() << endl;
        for(const auto& thing : data) cout << thing.second.size() << std::endl;
    }

    for(int i=0; i<NUM_CH; i++)
    {
        if(data[i].size()!=NUM_SAMP)
        {
            cout << "error 2: not 256 samples in channel " << i << endl;
        }
    }

    return 0;
}

void ACDC::createFile(const std::string& fname)
{
    if(outFile_)
    {
        outFile_->close();
    }
    outFile_.reset(new ofstream(fname + to_string(boardIndex) + ".txt", ios::app | ios::binary)); 
}

void ACDC::writeRawDataToFile(const vector<uint64_t>& buffer) const//, string rawfn)
{
    if(outFile_) outFile_->write(reinterpret_cast<const char*>(buffer.data()), buffer.size()*sizeof(uint64_t));
    else         writeErrorLog("No File!!!");
    return;
}

void ACDC::writeErrorLog(string errorMsg) const
{
    string err = "errorlog.txt";
    cout << "------------------------------------------------------------" << endl;
    cout << errorMsg << endl;
    cout << "------------------------------------------------------------" << endl;
    ofstream os_err(err, ios_base::app);
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
//    ss << std::put_time(std::localtime(&in_time_t), "%m-%d-%Y %X");
//    os_err << "------------------------------------------------------------" << endl;
//    os_err << ss.str() << endl;
//    os_err << errorMsg << endl;
//    os_err << "------------------------------------------------------------" << endl;
//    os_err.close();
}









