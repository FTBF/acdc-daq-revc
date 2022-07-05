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

int main()
{
	int retval;
	bool flag = true;

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

	float validationStart;
	float validationWindow;

	int psec_chip;
	unsigned int psec_channel;
	
	int ACCsw;
	int ACDCsw;
	
	std::vector<int> vec_psec_chip = {1,2,3,4,5};
	std::vector<unsigned int> vec_psec_channel = {0x3f, 0x3f, 0x3f, 0x3f, 0x3f};

	int BeamgateMultiplexer;
	int PPS_divide_ratio;
	
	int metaSwitch;

	ACC acc;

	system("mkdir -p Results");
	
//	while(true)
//	{
//		std::cout << "Do you want to reset the ACC before use (0/1)?" << std::endl; 
//		std::cout << "Set it to: ";
//		std::cin >> ACCsw;  cin.ignore(numeric_limits<streamsize>::max(),'\n');	
//		if(ACCsw==1)
//		{
//			acc.resetACC();
//		}
//		
//		std::cout << "Do you want to reset all ACDCs before use (0/1)?" << std::endl; 
//		std::cout << "Set it to: ";
//		std::cin >> ACDCsw;  cin.ignore(numeric_limits<streamsize>::max(),'\n');	
//		if(ACDCsw==1)
//		{
//			acc.resetACDC();
//		}
//		
//		break;
//	}
		
	while(flag)
	{
		std::cout << "Please select triggermode: " << std::endl; 
		std::cout << "(0) Off" << std::endl;
		std::cout << "(1) Software trigger" << std::endl;
		std::cout << "(2) Self-trigger" << std::endl;
                std::cout << "(3) Self-trigger with validation ACC" << std::endl;
                std::cout << "(4) ACC hardware trigger with perferal software trigger" << std::endl;
		std::cout << "Set it to: ";

		std::cin >> triggermode;
		cin.ignore(numeric_limits<streamsize>::max(),'\n');

		if(triggermode==1)
		{
			break;
		}

		switch(triggermode)
		{
                    
			case 2:
                        case 3:
				//std::cout << "Use normal polarity (0, high level or rising edge) or inverted polarity (1, low level or falling edge) on ACC SMA validation?" << std::endl;
				//std::cout << "Enter: ";
				//cin >> invertMode;	cin.ignore(numeric_limits<streamsize>::max(),'\n');

				acc.setSign(0, 2);
                                if(triggermode == 2) goto selfsetup;

				//std::cout << "How long should the validation window start be delayed from 0 to 819us in 25ns steps?" << std::endl;
				//std::cout << "Enter in ns: ";
				//cin >> validationStart;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				//ss3 << std::hex << (int)(validationStart/25);
				//valstr = std::stoul(ss3.str(),nullptr,16);
                                //
				//std::cout << "How long should the validation window be from 0 to 819us in 25ns steps?" << std::endl;
				//std::cout << "Enter in ns: ";
				//cin >> validationWindow;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				//ss4 << std::hex << (int)(validationWindow/25);
				//valstr2 = std::stoul(ss4.str(),nullptr,16);

//				std::cout << "Do you want to enable the PPS/Beamgate multiplexer? (0/1)" << std::endl;
//				std::cout << "Enter : ";
//				cin >> BeamgateMultiplexer;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
//		
				//acc.setValidationStart(valstr);
				//acc.setValidationWindow(valstr2);
//				acc.setPPSBeamMultiplexer(BeamgateMultiplexer);

				goto selfsetup;
                        case 4:
                                //hi there
                                goto selfsetup;
                                
			default:
				std::cout << " Trigger input not found " << std::endl;
				break;
			selfsetup:
				//std::cout << "Use normal polarity (0, high level or rising edge) or inverted polarity (1, low level or falling edge) for selftrigger?" << std::endl;
				//std::cout << "Enter: ";
				//cin >> invertMode;	cin.ignore(numeric_limits<streamsize>::max(),'\n');

//				std::cout << "Enable coincidence for self trigger? (0/1)" << std::endl;
//				std::cout << "Enter: ";
//				cin >> enableCoin;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
//				if(enableCoin == 1)
//				{
//					std::cout << "How many of 30 ch need to be in coincidence to self trigger? (0-30)" << std::endl;
//					std::cout << "Enter: ";
//					cin >> ChCoin;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
//					stringstream ss;
//					unsigned int hexstr;
//					ss << ChCoin;
//					ss >> std::hex >> hexstr;
//					acc.setNumChCoin(hexstr);
//				}

				std::cout << "Set the threshold of the self trigger in adc counts from 0 to 4095" << std::endl;
				std::cout << "Enter: ";
				cin >> threshold;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				stringstream ss2;
				unsigned int adcstr;
				ss2 << threshold;
				ss2 >> std::hex >> adcstr;

				//while(run<5)
				//{
				//	std::cout << "Do you want to enable channels on Psec chip " << run << "? (0/1)" << std::endl;
				//	std::cout << "Enter: ";
				//	cin >> psec_chip;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				//	if(psec_chip==1)
				//	{
				//		std::cout << "What channels of psec chip" << run << "do you want to enable? (From 0x00 to 0x3F, each bit representing a channel)" << std::endl;
				//		std::cout << "Enter: ";
				//		cin >> psec_channel;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
				//	}else
				//	{
				//		psec_channel = 0x00;
				//	}
				//	vec_psec_chip.push_back(psec_chip);
				//	vec_psec_channel.push_back(psec_channel);
				//	run++;
				//}

				acc.setThreshold(adcstr);
				//acc.setEnableCoin(enableCoin);
                                acc.setEnableCoin(0);
				//acc.setSign(invertMode, 4);
                                acc.setSign(0, 4);
				acc.setPsecChipMask(vec_psec_chip);
				acc.setPsecChannelMask(vec_psec_channel);

				flag = false;
				break;
		}
	}

//	std::cout << "What should be the PPS divide ratio in seconds?" << std::endl;
//	std::cout << "Enter in s: ";
//	cin >> PPS_divide_ratio;	cin.ignore(numeric_limits<streamsize>::max(),'\n');
//	ss5 << std::hex << (int)(PPS_divide_ratio);
//	valstr3 = std::stoul(ss5.str(),nullptr,16);
//	acc.setPPSRatio(valstr3);

//	while(true)
//	{
//		std::cout << "Please select which boards to use from 0x00 to 0xFF (default)" << std::endl;
//		
//		std::cin >> bbm;
//		cin.ignore(numeric_limits<streamsize>::max(),'\n');
//
//		if(bbm.size()>0)
//		{
//			boardmask = std::stoul(bbm,nullptr,16);
//			break;
//		}
//	}
        boardmask = 0xff;
//
//	while(true)
//	{
//		std::cout << "Do you want to use the calibration input mode? (0/1)" << std::endl;
//
//		cin >> calibMode;
//		cin.ignore(numeric_limits<streamsize>::max(),'\n');
//
//		if(calibMode==0 || calibMode==1)
//		{
//			break;
//		}
//	}
        calibMode = 0;
//
//	while(true)
//	{
//		std::cout << "Do you want to use raw mode(0/1)? Yes means you get a file with 7795 words in hex, No will give the usual data format." << std::endl;
//
//		cin >> rawMode;
//		cin.ignore(numeric_limits<streamsize>::max(),'\n');
//
//		if(rawMode==0)
//		{
//			rawBool = false;
//			std::cout << "Do you want to parse Metadata as well?" << std::endl;
//			
//			cin >> metaSwitch;
//			cin.ignore(numeric_limits<streamsize>::max(),'\n');
//			acc.setMetaSwitch(metaSwitch);
//			break;
//		}else if(rawMode==1)
//		{
//			rawBool = true;
//			break;
//		}
//	}
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
		retval = acc.listenForAcdcData(triggermode, rawBool, timestamp);
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


