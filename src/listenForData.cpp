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
#include <memory>
#include <thread>
#include <stdio.h>
#include <getopt.h>

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

//based on answer from https://stackoverflow.com/questions/41326112/how-to-merge-node-in-yaml-cpp
YAML::Node merge_nodes(const YAML::Node& a, const YAML::Node& b)
{
    if (!b.IsMap()) {
        // If b is not a map, merge result is b, unless b is null
        return b.IsNull() ? a : b;
    }
    if (!a.IsMap()) {
        // If a is not a map, merge result is b
        return b;
    }
    if (!b.size()) {
        // If a is a map, and b is an empty map, return a
        return a;
    }
    // Create a new map 'c' with the same mappings as a, merged with b
    auto c = YAML::Node(YAML::NodeType::Map);
    for (auto n : a) {
        if (n.first.IsScalar()) {
            const std::string & key = n.first.Scalar();
            auto t = YAML::Node(b[key]);
            if (t) {
                c[n.first] = merge_nodes(n.second, t);
                continue;
            }
        }
        c[n.first] = n.second;
    }
    // Add the mappings from 'b' not already in 'c'
    for (auto n : b) {
        if (!n.first.IsScalar() || !c[n.first.Scalar()]) {
            c[n.first] = n.second;
        }
    }
    return c;
}

int main(int argc, char *argv[])
{
    std::string configFile = "test.yaml";
    bool sequence = false;
    bool scan = false;
    std::string ip_cl;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"cfg",     required_argument, 0,  'c' },
            {"ip",      required_argument, 0,  'i' },
            {0,         0,                 0,  0 }
        };

        int c = getopt_long(argc, argv, "c:i:", long_options, &option_index);
        if (c == -1)
            break;

        switch (option_index) 
        {
        case 0:
            configFile = optarg;
            break;

        case 1:
            ip_cl = optarg;
            break;
        default:
            printf("?? getopt returned character code 0%o ??\n", c);
        }
    }

    printf("Using config file: %s\n", configFile.c_str());

    YAML::Node config = YAML::LoadFile(configFile);

    YAML::Node config_DAQ = config["basicConfig"]["DAQSettings"];

    int retval;
    string timestamp;

    std::vector<YAML::Node> ACC_configs;
    if(config["ACC1"])
    {
        int iACC = 1;
        while(config["ACC" + std::to_string(iACC)])
        {
            ACC_configs.push_back(merge_nodes(YAML::Clone(config_DAQ), config["ACC" + std::to_string(iACC)]["DAQSettings"]));
            if(ACC_configs.back()["fileLabel"]) ACC_configs.back()["fileLabel"] = ACC_configs.back()["fileLabel"].as<std::string>() + "_ACC" + std::to_string(iACC);
            else                                ACC_configs.back()["fileLabel"] = "ACC" + std::to_string(iACC);
            
            ++iACC;      
        }
    }
    else
    {
        ACC_configs.push_back(config_DAQ);
    }
        
    std::vector<std::vector<YAML::Node>> configs;

    //if a sequence or scan is requested, prepare this
    if(config["sequence"])
    {
        for(const auto& cfg : config["sequence"])
        {
            if(cfg.first.as<std::string>() == "scan" && cfg.second["DAQSettings"])
            {
                //a scan should only list one variable for scanning
                std::string variable;
                for(const auto& var : cfg.second["DAQSettings"])
                {
                    variable = var.first.as<std::string>();
                    break;
                }
                int start = cfg.second["DAQSettings"][variable]["start"].as<int>();
                int stop  = cfg.second["DAQSettings"][variable]["stop"].as<int>();
                int step  = cfg.second["DAQSettings"][variable]["step"].as<int>();

                for(int iVar = start; iVar <= stop; iVar += step)
                {
                    configs.push_back({});
                    auto& acc_cfg = configs.back();
                    for(const auto& acc_base_cfg : ACC_configs)
                    {
                        acc_cfg.push_back(YAML::Clone(acc_base_cfg));
                        acc_cfg.back()[variable] = iVar;
                        if(acc_cfg.back()["fileLabel"]) acc_cfg.back()["fileLabel"] = acc_cfg.back()["fileLabel"].as<std::string>() + "_scan_" + variable + "_" + std::to_string(iVar);
                        else                            acc_cfg.back()["fileLabel"] = "scan_" + variable + "_" + std::to_string(iVar);
                        printf("iVar: %d, %d\n", iVar, acc_cfg.size());
                    }
                }
            }
//                else
//                {
//                    acc_cfg.push_back(merge_nodes(YAML::Clone(acc_base_cfg), cfg.second["DAQSettings"]));
//                    if(acc_cfg.back()["fileLabel"]) acc_cfg.back()["fileLabel"] = acc_cfg.back()["fileLabel"].as<std::string>() + "_" + cfg.first.as<std::string>();
//                    else                            acc_cfg.back()["fileLabel"] = cfg.first.as<std::string>();
//                }
        }
    }
    else
    {
        configs.emplace_back(ACC_configs);
    }

    //for(const auto& config : configs)
    //{    
    //    std::ofstream fout(config["fileLabel"].as<std::string>() + ".yaml");
    //    fout << config;
    //}

    timestamp = getTime();
    std::chrono::high_resolution_clock::time_point t0;
    for(const auto& config_acc_custom : configs)
    {
        std::vector<std::unique_ptr<ACC>> acc_vec;

        for(const auto& config_custom : config_acc_custom )
        {
            std::string ip;
            if(ip_cl.size()) ip = ip_cl;
            else             ip = config_custom["ip"].as<std::string>();

            printf("ip: %s\n", ip.c_str());
            acc_vec.emplace_back(new ACC(ip));

            //system("mkdir -p Results");

            retval = acc_vec.back()->initializeForDataReadout(config_custom, timestamp);
            if(retval != 0)
            {
                cout << "Initialization failed!" << endl;
                return 0;
            }
            acc_vec.back()->dumpData(0xFF);
        }

//        eventCounter = 0;
//        failCounter = 0;
//        int reTime = 500;
//        int mult = 1;
        t0 = std::chrono::high_resolution_clock::now();

        for(auto& acc : acc_vec)
        {
            acc->startDAQThread();
        }
        for(auto& acc : acc_vec)
        {
            acc->joinDAQThread();
        }
        //acc.listenForAcdcData();
        //acc.endRun();
        
        auto t1 = std::chrono::high_resolution_clock::now();
        auto dt = 1.e-9*std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
        cout << "It took "<< dt <<" second(s)."<< endl;
    }
	
    return 1;
}


