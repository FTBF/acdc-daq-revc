#include "Metadata.h"
#include <algorithm>
#include <sstream>
#include <bitset>
#include <iostream>
#include <cmath>
#include <chrono> 
#include <iomanip>
#include <numeric>
#include <ctime>

using namespace std;

Metadata::Metadata()
{
	//initializeMetadataKeys();
}

Metadata::Metadata(vector<unsigned short> acdcBuffer)
{
    (void)acdcBuffer;
	//initializeMetadataKeys();
	//parseBuffer(acdcBuffer);
}

Metadata::~Metadata()
{
}


//----------Massive parsing functions are below. 
//two metadatas that are known externally need to be set by ACDC class.
void Metadata::setBoardAndEvent(unsigned short board, int event)
{
    //there is a problem in that the map has been
    //defined assuming all metadata are unsigned shorts. 
    //but this only goes to 2^16 events (65536). If this happens, 
    //restart the count at zero
    event = event % 65536;
	checkAndInsert("Event", (unsigned short)event);
	checkAndInsert("Board", board);
}

int Metadata::getEventNumber()
{
    return -1;
}

//takes the full acdcBuffer as input. 
//splits it into a map[key][vector<unsigned short>]
//which is then parsed and organized in this class. 
//Returns:
//false if a corrupt buffer happened
//true if all good. 
int Metadata::parseBuffer(vector<unsigned short> buffer, unsigned short bi)
{
	//Catch empty buffers
	if(buffer.size() == 0)
	{
		std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
		return -1;
	}

 	//Prepare the Metadata vector | global vector vector<unsigned short> meta 
	meta.clear();

	//Helpers
	int DistanceFromZero;
	int chip_count = 0;
	
	//Indicator words for the start/end of the metadata
	const unsigned short startword = 0xBA11; 
	unsigned short endword = 0xFACE; 
    	unsigned short endoffile = 0x4321;

	//Empty metadata map for each Psec chip <PSEC #, vector with information>
	map<int, vector<unsigned short>> PsecInfo;

	//Empty trigger metadata map for each Psec chip <PSEC #, vector with trigger>
	map<int, vector<unsigned short>> PsecTriggerInfo;
	unsigned short CombinedTriggerRateCount;

	//Empty vector with positions of aboves startword
	vector<int> start_indices; 

	//Find the startwords and write them to the vector
	vector<unsigned short>::iterator bit;
	for(bit = buffer.begin(); bit != buffer.end(); ++bit)
	{
        	if(*bit == startword)
        	{
        		DistanceFromZero= std::distance(buffer.begin(), bit);
        		start_indices.push_back(DistanceFromZero);
        	}
	}

	//Filter in cases where one of the start words is found in the metadata 
	if(start_indices.size()>NUM_PSEC)
	{
		for(int k=0; k<(int)start_indices.size()-1; k++)
		{
		    	if(start_indices[k+1]-start_indices[k]>6*256+14)
		    	{
				//nothing
		    	}else
		    	{
				start_indices.erase(start_indices.begin()+(k+1));
				k--;
		    	}
		}
	}

	//Last case emergency stop if metadata is still not quite right
	if(start_indices.size() != NUM_PSEC)
	{
        	string fnnn = "meta-corrupt-psec-buffer.txt";
        	cout << "Printing to file : " << fnnn << endl;
        	ofstream cb(fnnn);
        	for(unsigned short k: buffer)
        	{
            		cb << hex << k << endl;
        	}
        	return -2;
	}

	//Fill the psec info map
	for(int i: start_indices)
	{
		//Write the first word after the startword
		bit = buffer.begin() + (i+1);

		//As long as the endword isn't reached copy metadata words into a vector and add to map
		vector<unsigned short> InfoWord;
		while(*bit != endword && *bit != endoffile && InfoWord.size() < 14)
		{
			InfoWord.push_back(*bit);
			++bit;
		}
		PsecInfo.insert(pair<int, vector<unsigned short>>(chip_count, InfoWord));
		chip_count++;
	}

	//Fill the psec trigger info map
	for(int chip=0; chip<NUM_PSEC; chip++)
	{
	    for(int ch=0; ch<NUM_CH/NUM_PSEC; ch++)
	    {
	    	//Find the trigger data at begin + last_metadata_start + 13_info_words + 1_end_word + 1 
	        bit = buffer.begin() + start_indices[4] + 13 + 1 + 1 + ch + (chip*(NUM_CH/NUM_PSEC));
	        PsecTriggerInfo[chip].push_back(*bit);
	    }
	}

	//Fill the combined trigger
	CombinedTriggerRateCount = buffer[7792];

	//----------------------------------------------------------

  	//Start the metadata parsing 
	meta.push_back(bi);
	for(int CHIP=0; CHIP<NUM_PSEC; CHIP++)
	{
		meta.push_back((0xDCB0 | CHIP));
		for(int INFOWORD=0; INFOWORD<13; INFOWORD++)
		{
			meta.push_back(PsecInfo[CHIP][INFOWORD]);		
		}
		for(int TRIGGERWORD=0; TRIGGERWORD<6; TRIGGERWORD++)
		{
			meta.push_back(PsecTriggerInfo[CHIP][TRIGGERWORD]);
		}
	}

	meta.push_back(CombinedTriggerRateCount);
	meta.push_back(0xeeee);
	return 0;
}


//just makes sure not to insert elements
//into metadata map if they already exist. 
void Metadata::checkAndInsert(string key, unsigned short val)
{
    (void) key;
    (void)val;
}



//keeps the metadata strings in a consistent
//order. Initialized in constructor and is used
//to order the printing/output of metadata map. 
void Metadata::initializeMetadataKeys()
{
    metadata_keys.push_back("Event");
    metadata_keys.push_back("Board");
    //General PSEC info 
    for(int i = 0; i < NUM_PSEC; i++)
    {
        metadata_keys.push_back("feedback_count_"+to_string(i)); //Info 1
        metadata_keys.push_back("feedback_target_count_"+to_string(i)); //Info 2
        metadata_keys.push_back("Vbias_setting_"+to_string(i)); //Info 3
        metadata_keys.push_back("selftrigger_threshold_setting_"+to_string(i)); //Info 4
        metadata_keys.push_back("PROVDD_setting_"+to_string(i)); // Info 5
        metadata_keys.push_back("VCDL_count_lo_"+to_string(i)); //Info 11, later bit(15-0)
        metadata_keys.push_back("VCDL_count_hi_"+to_string(i)); //Info 12, later bit(31-16)
        metadata_keys.push_back("DLLVDD_setting_"+to_string(i)); //Info 13
    }

    //Trigger PSEC settings
    metadata_keys.push_back("trigger_mode"); //Info 6, PSEC1 bit(3-0)
    //metadata_keys.push_back("trigger_validation_window_start"); //Info 6, PSEC1 bit(15-4)
    //metadata_keys.push_back("trigger_validation_window_length"); //info 6, PSEC2 bit(11-0)
    metadata_keys.push_back("trigger_sma_invert"); // Info 6, PSEC3 bit(1)
    //metadata_keys.push_back("trigger_sma_detection_mode"); // Info 6, PSEC3 bit(0)
    //metadata_keys.push_back("trigger_acc_invert"); // Info 6, PSEC3 bit(3)
    //metadata_keys.push_back("trigger_acc_detection_mode"); // Info 6, PSEC3 bit(2)
    metadata_keys.push_back("trigger_self_sign"); //Info 6, PSEC3 bit(5)
    //metadata_keys.push_back("trigger_self_detection_mode"); // Info 6, PSEC3 bit(4)
    metadata_keys.push_back("trigger_self_coin"); // Info 6, PSEC3 bit(10-6)

    metadata_keys.push_back("trigger_selfmask_0"); //Info 7 PSEC0
    metadata_keys.push_back("trigger_selfmask_1"); //Info 7 PSEC1
    metadata_keys.push_back("trigger_selfmask_2"); //Info 7 PSEC2
    metadata_keys.push_back("trigger_selfmask_3"); //Info 7 PSEC3
    metadata_keys.push_back("trigger_selfmask_4"); //Info 7 PSEC4

    metadata_keys.push_back("trigger_self_threshold_0"); //Info 8 PSEC0 bit(11-0)
    metadata_keys.push_back("trigger_self_threshold_1"); //Info 8 PSEC1 bit(11-0)
    metadata_keys.push_back("trigger_self_threshold_2"); //Info 8 PSEC2 bit(11-0)
    metadata_keys.push_back("trigger_self_threshold_3"); //Info 8 PSEC3 bit(11-0)
    metadata_keys.push_back("trigger_self_threshold_4"); //Info 8 PSEC4 bit(11-0)

    //Timestamp data
    metadata_keys.push_back("timestamp_0"); // Info 9 PSEC0 later bit(15-0)
    metadata_keys.push_back("timestamp_1"); // Info 9 PSEC1 later bit(31-16)
    metadata_keys.push_back("timestamp_2"); // Info 9 PSEC2 later bit(47-32)
    metadata_keys.push_back("timestamp_3"); // Info 9 PSEC3 later bit(63-48)

    metadata_keys.push_back("clockcycle_bits"); // Info 9 PSEC0 bit(2-0)

    //Event count
    metadata_keys.push_back("event_count_lo"); //Info 10 PSEC0 later bit(15-0)
    metadata_keys.push_back("event_count_hi"); //Info 10 PSEC1 later bit(31-16)

    for(int ch=0; ch<NUM_CH; ch++)
    {
         metadata_keys.push_back("self_trigger_rate_count_psec_ch"+to_string(ch));
    }
    metadata_keys.push_back("combined_trigger_rate_count");
}

void Metadata::writeErrorLog(string errorMsg)
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
    os_err.close();
}
