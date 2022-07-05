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

using namespace std;

string getTime()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    //    ss << std::put_time(std::localtime(&in_time_t), "%Y%d%m_%H%M%S");
    return ss.str();
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

map<string, unsigned int> parseInfos(vector<unsigned short> buffer, int evn)
{
    map<string, unsigned int> tmpMap;
  
    if(buffer.size()==7795)
    {
	tmpMap["EVENTNBR"] = evn;
	tmpMap["fTYPE"] = buffer.at(1);
	tmpMap["fLENGTH"] = buffer.size();
	tmpMap["fTIMESTAMP0"] = buffer.at(1548);
	tmpMap["fTIMESTAMP1"] = buffer.at(3100);
	tmpMap["fTIMESTAMP2"] = buffer.at(4652);
	tmpMap["fTIMESTAMP3"] = buffer.at(6204);    
	tmpMap["fCOUNTER0"] = buffer.at(1549);  
	tmpMap["fCOUNTER1"] = buffer.at(3101);  
    }else if(buffer.size()==16)
    {
	tmpMap["EVENTNBR"] = evn;
	tmpMap["fTYPE"] =  buffer.at(1);
	tmpMap["fLENGTH"] = buffer.size();    
	tmpMap["fTIMESTAMP0"] = buffer.at(5);
	tmpMap["fTIMESTAMP1"] = buffer.at(4);
	tmpMap["fTIMESTAMP2"] = buffer.at(3);
	tmpMap["fTIMESTAMP3"] = buffer.at(2); 
	tmpMap["fCOUNTER0"] = buffer.at(7);  
	tmpMap["fCOUNTER1"] = buffer.at(6);  
    }else{
	    std::cout << "Incompatible size" << endl;
    	    return {};
    }
    return tmpMap;
}

void printInfos(map<string, unsigned int> datamap)
{
    std::string dtf = "DataMapInfos.txt";
    std::string type;
    unsigned long long timestamp; 
    unsigned int sn;
    stringstream ss_TS;
    stringstream ss_EC;
     
    if(datamap["fTYPE"] == 0xeeee)
    {
        type = "PPS"; 
    }else if(datamap["fTYPE"] == 0xa5ec)
    {
        type = "PSEC";
    }else
    {
        type = "INVALID";
    }

    ss_TS << std::setfill('0') << std::setw(4) << std::hex << datamap["fTIMESTAMP3"];
    ss_TS << std::setfill('0') << std::setw(4) << std::hex << datamap["fTIMESTAMP2"];
    ss_TS << std::setfill('0') << std::setw(4) << std::hex << datamap["fTIMESTAMP1"];
    ss_TS << std::setfill('0') << std::setw(4) << std::hex << datamap["fTIMESTAMP0"];
    timestamp = std::stoull(ss_TS.str(),nullptr,16);
  
    ss_EC << std::setfill('0') << std::setw(4) << std::hex << datamap["fCOUNTER1"];
    ss_EC << std::setfill('0') << std::setw(4) << std::hex << datamap["fCOUNTER0"];	
    sn = std::stoull(ss_EC.str(),nullptr,16);
	
    std::cout << "------------------------------------------------------------" << std::endl;
    std::cout << "Event " << datamap["EVENTNBR"] << ": TYPE = " << type << " , S/N = " << sn << " , LENGTH = " <<  datamap["fLENGTH"] << " with TIMESTAMP = " << timestamp << " = " << (float)timestamp/320000000 << " s" << endl;
	
	
    //ofstream os_dtf(dtf, ios_base::app);
    //os_dtf << datamap["EVENTNBR"] << " " << type << " " <<  datamap["fLENGTH"] << " " << (float)timestamp/320000000 << endl;
    //os_dtf.close();
}

int main()
{
	int retval;
	bool flag = true;
 	vector<unsigned short> buffer;
 	map<string, unsigned int> datamap;

	int triggermode;

	string bbm;
	unsigned int boardmask;

	int calibMode;

	int rawMode;
	bool rawBool;

	int invertMode;

	int ChCoin;
	int enableCoin;

	int eventCounter;
	int eventNumber;
	int failCounter;
	
	int threshold;

	stringstream ss3;
	stringstream ss4;
	stringstream ss5;

	unsigned int valstr;
	unsigned int valstr2;
	unsigned int valstr3;

	string timestamp;

	int run=0;

	int validationStart;
	int validationWindow;

	int psec_chip;
	unsigned int psec_channel;
	
	std::vector<int> vec_psec_chip;
	std::vector<unsigned int> vec_psec_channel;

	int BeamgateMultiplexer;
	int PPS_divide_ratio;

	ACC acc;

	system("mkdir -p Results");
	
	while(flag)
	{
		std::cout << "Please select triggermode: " << std::endl; 
		std::cout << "(0) Off" << std::endl;
		std::cout << "(1) Software trigger" << std::endl;
		std::cout << "(2) SMA trigger ACC" << std::endl;
		std::cout << "(3) SMA trigger ACDC" << std::endl;
		std::cout << "(4) Self-trigger" << std::endl;
		std::cout << "(5) Self-trigger with validation ACC" << std::endl;
		std::cout << "(6) Self-trigger with validation ACDC" << std::endl;
		std::cout << "(7) SMA ACC with validation SMA ACDC" << std::endl;
		std::cout << "(8) SMA ACDC with validation SMA ACC" << std::endl;
		std::cout << "(9) Pulse-per-second trigger mode" << std::endl;
		std::cout << "Set it to: ";

		std::cin >> triggermode;
		cin.ignore(numeric_limits<streamsize>::max(),'\n');

		if(triggermode==1 || triggermode==9)
		{
			break;
		}

		switch(triggermode)
		{
			case 2:
				std::cout << "Use normal polarity (0, high level or rising edge) or inverted polarity (1, low level or falling edge) on ACC SMA?" << std::endl;
				std::cout << "Enter: ";
				cin >> invertMode; 		cin.ignore(numeric_limits<streamsize>::max(),'\n');

				acc.setSign(invertMode, 2);

				flag = false;
				break;
			case 3:
				std::cout << "Use normal polarity (0, high level or rising edge) or inverted polarity (1, low level or falling edge) on ACDC SMA?" << std::endl;
				std::cout << "Enter: ";
				cin >> invertMode;		cin.ignore(numeric_limits<streamsize>::max(),'\n');

				acc.setSign(invertMode, 3);

				flag = false;
				break;
			case 4:
				goto selfsetup;
			case 5: 
				std::cout << "Use normal polarity (0, high level or rising edge) or inverted polarity (1, low level or falling edge) on ACC SMA validation?" << std::endl;
				std::cout << "Enter: ";
				cin >> invertMode;	cin.ignore(numeric_limits<streamsize>::max(),'\n');

				std::cout << "How long should the validation window start be delayed from 0 to 819us in 25ns steps?" << std::endl;
				std::cout << "Enter in ns: ";
				cin >> validationStart;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				ss3 << std::hex << (int)(validationStart/25);
				valstr = std::stoul(ss3.str(),nullptr,16);

				std::cout << "How long should the validation window be from 0 to 819us in 25ns steps?" << std::endl;
				std::cout << "Enter in ns: ";
				cin >> validationWindow;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				ss4 << std::hex << (int)(validationWindow/25);
				valstr2 = std::stoul(ss4.str(),nullptr,16);

				std::cout << "Do you want to enable the PPS/Beamgate multiplexer? (0/1)" << std::endl;
				std::cout << "Enter : ";
				cin >> BeamgateMultiplexer;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
		
				acc.setValidationStart(valstr);
				acc.setValidationWindow(valstr2);
				acc.setPPSBeamMultiplexer(BeamgateMultiplexer);
				acc.setSign(invertMode, 2);

				goto selfsetup;
			case 6:
				std::cout << "Use normal polarity (0, high level or rising edge) or inverted polarity (1, low level or falling edge) on ACDC SMA validation?" << std::endl;
				std::cout << "Enter: ";
				cin >> invertMode;	cin.ignore(numeric_limits<streamsize>::max(),'\n');

				std::cout << "How long should the validation window start be delayed from 0 to 819us in 25ns steps?" << std::endl;
				std::cout << "Enter in ns: ";
				cin >> validationStart;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				ss3 << std::hex << (int)(validationStart/25);
				valstr = std::stoul(ss3.str(),nullptr,16);

				std::cout << "How long should the validation window be from 0 to 819us in 25ns steps?" << std::endl;
				std::cout << "Enter in ns: ";
				cin >> validationWindow;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				ss4 << std::hex << (int)(validationWindow/25);
				valstr2 = std::stoul(ss4.str(),nullptr,16);

				acc.setValidationStart(valstr);
				acc.setValidationWindow(valstr2);
				acc.setSign(invertMode, 3);

				goto selfsetup;
			case 7:
				std::cout << "Use normal polarity (0, high level or rising edge) or inverted polarity (1, low level or falling edge) on ACC SMA?" << std::endl;
				std::cout << "Enter: ";
				cin >> invertMode;	cin.ignore(numeric_limits<streamsize>::max(),'\n');

				acc.setSign(invertMode, 2);

				std::cout << "Use normal polarity (0, high level or rising edge) or inverted polarity (1, low level or falling edge) on ACDC SMA validation?" << std::endl;
				std::cout << "Enter: ";
				cin >> invertMode;	cin.ignore(numeric_limits<streamsize>::max(),'\n');

				acc.setSign(invertMode, 3);

				std::cout << "How long should the validation window start be delayed from 0 to 819us in 25ns steps?" << std::endl;
				std::cout << "Enter in ns: ";
				cin >> validationStart;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				ss3 << std::hex << (int)(validationStart/25);
				valstr = std::stoul(ss3.str(),nullptr,16);

				std::cout << "How long should the validation window be from 0 to 819us in 25ns steps?" << std::endl;
				std::cout << "Enter in ns: ";
				cin >> validationWindow;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				ss4 << std::hex << (int)(validationWindow/25);
				valstr2 = std::stoul(ss4.str(),nullptr,16);

				acc.setValidationStart(valstr);
				acc.setValidationWindow(valstr2);

				flag = false;

				break;
			case 8:
				std::cout << "Use normal polarity (0, high level or rising edge) or inverted polarity (1, low level or falling edge) on ACDC SMA?" << std::endl;
				std::cout << "Enter: ";
				cin >> invertMode;	cin.ignore(numeric_limits<streamsize>::max(),'\n');

				acc.setSign(invertMode, 3);

				std::cout << "Use normal polarity (0, high level or rising edge) or inverted polarity (1, low level or falling edge) on ACC SMA validation?" << std::endl;
				std::cout << "Enter: ";
				cin >> invertMode;	cin.ignore(numeric_limits<streamsize>::max(),'\n');

				acc.setSign(invertMode, 2);


				std::cout << "How long should the validation window start be delayed from 0 to 819us in 25ns steps?" << std::endl;
				std::cout << "Enter in ns: ";
				cin >> validationStart;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				ss3 << std::hex << (int)(validationStart/25);
				valstr = std::stoul(ss3.str(),nullptr,16);

				std::cout << "How long should the validation window be from 0 to 819us in 25ns steps?" << std::endl;
				std::cout << "Enter in ns: ";
				cin >> validationWindow;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				ss4 << std::hex << (int)(validationWindow/25);
				valstr2 = std::stoul(ss4.str(),nullptr,16);

				std::cout << "Do you want to enable the PPS/Beamgate multiplexer? (0/1)" << std::endl;
				std::cout << "Enter : ";
				cin >> BeamgateMultiplexer;	cin.ignore(numeric_limits<streamsize>::max(),'\n');

				acc.setValidationStart(valstr);
				acc.setValidationWindow(valstr2);
				acc.setPPSBeamMultiplexer(BeamgateMultiplexer);

				flag = false;

				break;
			default:
				std::cout << " Trigger input not found " << std::endl;
				break;
			selfsetup:
				std::cout << "Use normal polarity (0, high level or rising edge) or inverted polarity (1, low level or falling edge) for selftrigger?" << std::endl;
				std::cout << "Enter: ";
				cin >> invertMode;	cin.ignore(numeric_limits<streamsize>::max(),'\n');

				std::cout << "Enable coincidence for self trigger? (0/1)" << std::endl;
				std::cout << "Enter: ";
				cin >> enableCoin;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				if(enableCoin == 1)
				{
					std::cout << "How many of 30 ch need to be in coincidence to self trigger? (0-30)" << std::endl;
					std::cout << "Enter: ";
					cin >> ChCoin;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
					stringstream ss;
					unsigned int hexstr;
					ss << ChCoin;
					ss >> std::hex >> hexstr;
					acc.setNumChCoin(hexstr);
				}

				std::cout << "Set the threshold of the self trigger in adc counts from 0 to 4095" << std::endl;
				std::cout << "Enter: ";
				cin >> threshold;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				stringstream ss2;
				unsigned int adcstr;
				ss2 << threshold;
				ss2 >> std::hex >> adcstr;

				while(run<5)
				{
					std::cout << "Do you want to enable channels on Psec chip " << run << "? (0/1)" << std::endl;
					std::cout << "Enter: ";
					cin >> psec_chip;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
					if(psec_chip==1)
					{
						std::cout << "What channels of psec chip" << run << "do you want to enable? (From 0x00 to 0x3F, each bit representing a channel)" << std::endl;
						std::cout << "Enter: ";
						cin >> psec_channel;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
					}else
					{
						psec_channel = 0x00;
					}
					vec_psec_chip.push_back(psec_chip);
					vec_psec_channel.push_back(psec_channel);
					run++;
				}

				acc.setThreshold(adcstr);
				acc.setEnableCoin(enableCoin);
				acc.setSign(invertMode, 4);
				acc.setPsecChipMask(vec_psec_chip);
				acc.setPsecChannelMask(vec_psec_channel);

				flag = false;
				break;
		}
	}

	std::cout << "What should be the PPS divide ratio in seconds?" << std::endl;
	std::cout << "Enter in s: ";
	cin >> PPS_divide_ratio;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
	ss5 << std::hex << (int)(PPS_divide_ratio);
	valstr3 = std::stoul(ss5.str(),nullptr,16);
	acc.setPPSRatio(valstr3);

	boardmask = 0xFF;
	calibMode=0;
	rawBool = true;

	while(true)
	{
		std::cout << "How many events do you want to record?" << std::endl;

		cin >> eventNumber;
		cin.ignore(numeric_limits<streamsize>::max(),'\n');

		if(eventNumber>0)
		{
			flag = false;
			break;
		}
	}		
  
  //--------------------------------------------------------------------------------------------------------
	
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
	
  int reTime = 1000;
	int mult = 1;
	
  auto t0 = std::chrono::high_resolution_clock::now();

	while(eventCounter<eventNumber)
	{
		if(triggermode == 1)
		{
			acc.softwareTrigger();
		}
		if(eventCounter>=reTime*mult)
		{
			timestamp = getTime();
			mult++;
		}
		retval = acc.listenForAcdcData(triggermode, rawBool, timestamp);
		switch(retval)
		{
			case 0:
				
//				buffer = acc.returnRaw();
//				if(buffer.size()==0)
//				{
//					std::cout << "Empty buffer?" << std::endl;
//					break;
//				}
//				datamap = parseInfos(buffer,eventCounter);
//				printInfos(datamap);
//        
//				eventCounter++;
//				failCounter=0;
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
	
	int del;
	cout << endl;
	cout << "-------------------------------------------------------" << endl;
	cout << "Do you want to delete the saved files? If not please keep in mind the 32GB storage capacity!" << endl;
	cout << "Enter (0/1): ";
	cin >> del;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
	if(del==1){system("rm -r Results");}
	return 1;
}
