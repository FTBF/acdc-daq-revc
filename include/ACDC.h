#ifndef _ACDC_H_INCLUDEDreadDataFromFile
#define _ACDC_H_INCLUDED

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include "yaml-cpp/yaml.h"
#include "Metadata.h" //load metadata class

using namespace std;

#define NUM_CH 30 //maximum number of channels for one ACDC board
#define NUM_PSEC 5 //maximum number of psec chips on an ACDC board
#define NUM_SAMP 256 //maximum number of samples of one waveform
#define NUM_CH_PER_CHIP 6 //maximum number of channels per psec chips

class ACDC
{
public:
	ACDC(); //constructor
	ACDC(int bi); //constructor
	~ACDC(); //deconstructor

	//----------local return functions
	int getBoardIndex(); //get the current board index from the acdc
	int getNumCh() {int a = NUM_CH; return a;} //returns the number of total channels per acdc
	int getNumPsec() {int a = NUM_PSEC; return a;} //returns the number of psec chips on an acdc
	int getNumSamp() {int a = NUM_SAMP; return a;} //returns the number of samples for on event
	map<int, vector<unsigned short>> returnData(){return data;} //returns the entire data map | index: channel < samplevector
	map<string, unsigned short> returnMeta(){return map_meta;} //returns the entire meta map | index: metakey < value 

	//----------local set functions
	void setBoardIndex(int bi); // set the board index for the current acdc

    void parseConfig(const YAML::Node& config);

	//----------parse function for data stream 
	int parseDataFromBuffer(const vector<uint64_t>& buffer); //parses only the psec data component of the ACDC buffer

	//----------write data to file
	void writeErrorLog(string errorMsg); //write errorlog with timestamps

    class ConfigParams
    {
    public:
        ConfigParams();

        bool reset;
        std::vector<unsigned int> pedestals;
        int selfTrigPolarity;
        std::vector<unsigned int> triggerThresholds;
        std::vector<unsigned int> selfTrigMask;
        bool calibMode;
    } params_;

private:
	//----------all neccessary classes
	Metadata meta; //calls the metadata class for file write

	//----------all neccessary global variables
	int boardIndex; //var: represents the boardindex for the current board
	vector<unsigned short> lastAcdcBuffer; //most recently received ACDC buffer
	map<int, vector<unsigned short>> data; //entire data map | index: channel < samplevector
	map<string, unsigned short> map_meta; //entire meta map | index: metakey < value
};

#endif
