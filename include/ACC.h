#ifndef _ACC_H_INCLUDED
#define _ACC_H_INCLUDED

#include "ACDC.h" // load the ACDC class
#include "EthernetInterface.h"
#include <algorithm>
#include <vector>
#include <map>
#include "yaml-cpp/yaml.h"

using namespace std;

#define SAFE_BUFFERSIZE 100000 //used in setup procedures to guarantee a proper readout 
#define NUM_CH 30 //maximum number of channels for one ACDC board
#define MAX_NUM_BOARDS 8 // maxiumum number of ACDC boards connectable to one ACC 
#define ACCFRAME 32
#define ACDCFRAME 32
#define PPSFRAME 16
#define PSECFRAME 7696

class ACC
{
public:
	/*------------------------------------------------------------------------------------*/
	/*--------------------------------Constructor/Deconstructor---------------------------*/
	/*ID 5: Constructor*/
	ACC();
    ACC(const std::string& ip);
	/*ID 6: Constructor*/
	~ACC();

	/*------------------------------------------------------------------------------------*/
	/*--------------------------------Local return functions------------------------------*/
	/*ID Nan: Returns set triggermode */
	int getTriggermode(){return trigMode;} 
	/*ID Nan: Returns the data map*/
	map<int, map<int, vector<unsigned short>>> returnData(){return map_data;} 
	/*ID Nan: Returns the meta map*/
	map<int, vector<unsigned short>> returnMeta(){return map_meta;} 
	/*ID Nan: Returns the raw data vector*/
	vector<uint64_t> returnRaw(){return vbuffer;}
	/*ID Nan: Returns the acdc info frame map*/
	map<int, vector<unsigned short>> returnACDCIF(){return map_acdcIF;} 
	/*ID Nan: Returns the acc info frame map*/
	vector<unsigned short> returnACCIF(){return map_accIF;} 

	/*------------------------------------------------------------------------------------*/
	/*-------------------------Local set functions for board setup------------------------*/
	/*-------------------Sets global variables, see below for description-----------------*/
	void setValidationStart(unsigned int in){validation_start=in;} //sets the validation window start delay for required trigger modes
	void setValidationWindow(unsigned int in){validation_window=in;} //sets the validation window length for required trigger modes
	void setTriggermode(int in){trigMode = in;} //sets the overall triggermode
//	void setPPSRatio(unsigned int in){PPSRatio = in;} 
//	void setPPSBeamMultiplexer(int in){PPSBeamMultiplexer = in;} 
	void setMetaSwitch(int in){metaSwitch = in;}

	/*------------------------------------------------------------------------------------*/
	/*-------------------------Local set functions for board setup------------------------*/
    void parseConfig(const YAML::Node& config);
	/*ID:9 Create ACDC class instances for each connected ACDC board*/
	int createAcdcs(); 
	/*ID 10: Clear all ACDC class instances*/
	void clearAcdcs(); 
	/*ID:11 Queries the ACC for information about connected ACDC boards*/
    std::vector<int> whichAcdcsConnected(); 
	/*ID 12: Set up the software trigger*/
	void setSoftwareTrigger(unsigned int boardMask); 
	/*ID 13: Fires the software trigger*/
	void softwareTrigger(); 
	/*ID 15: Main listen fuction for data readout*/
    int listenForAcdcData(const string& timestamp="invalidTime"); 
	/*ID 16: Used to dis/enable transfer data from the PSEC chips to the buffers*/
	void enableTransfer(int onoff=0); 
	/*ID 17: Main init function that controls generalk setup as well as trigger settings*/
    int initializeForDataReadout(const YAML::Node& config);
	/*ID 18: Tells ACDCs to clear their ram.*/ 	
	void dumpData(unsigned int boardMask); 
	/*ID 19: Pedestal setting procedure.*/
	bool setPedestals(unsigned int boardmask, unsigned int chipmask, unsigned int adc); 
	/*ID 20: Switch for the calibration input on the ACC*/
	void toggleCal(int onoff, unsigned int channelmask = 0x7FFF,  unsigned int boardMask=0xFF); 
	/*ID 21: Set up the hardware trigger*/
	void setHardwareTrigSrc(int src, unsigned int boardMask = 0xFF); 
	/*ID 22: Special function to check the ports for connections to ACDCs*/
	void connectedBoards(); 
	/*ID 23: Wakes up the USB by requesting an ACC info frame*/
	void usbWakeup(); 
	/*ID 24: Special function to check connected ACDCs for their firmware version*/ 
	void versionCheck(bool debug = false);
	/*ID 25: Scan possible high speed link clock phases and select the optimal phase setting*/ 
	void scanLinkPhase(unsigned int boardMask, bool print = false);
        /*ID 26: Configure the jcPLL settings */
	void configJCPLL(unsigned int boardMask = 0xff);
        /*ID 27: Turn off triggers and data transfer off */
	void endRun();
	//:::
	void resetACDC(unsigned int boardMask = 0xff); //resets the acdc boards
	void resetACC(); //resets the acdc boards 

	/*------------------------------------------------------------------------------------*/
	/*--------------------------------------Write functions-------------------------------*/
	void writeErrorLog(string errorMsg); //writes an errorlog with timestamps for debugging
	void writePsecData(ofstream& d, const vector<int>& boardsReadyForRead); //main write for the data map
	void writeRawDataToFile(const vector<uint64_t>& buffer, string rawfn); //main write for the raw data vector

    class ConfigParams
    {
    public:
        ConfigParams();

        bool rawMode;
        int eventNumber;
        int triggerMode;
        unsigned int boardMask;
        std::string label;
        bool reset;
        int accTrigPolarity;
        int validationStart;
        int validationWindow;
    } params_;

private:
	/*------------------------------------------------------------------------------------*/
	/*---------------------------------Load neccessary classes----------------------------*/
        EthernetInterface eth, eth_burst;
	Metadata meta; //calls the metadata class for file write
    std::vector<ACDC> acdcs; //a vector of active acdc boards. 

	//----------all neccessary global variables
	bool usbcheck;
	int trigMode; //var: decides the triggermode
	int metaSwitch = 0;
	unsigned int validation_start;
	unsigned int validation_window; //var: validation window for some triggermodes
	unsigned int PPSRatio;
	int PPSBeamMultiplexer;
//	vector<unsigned short> lastAccBuffer; //most recently received ACC buffer
	map<int, map<int, vector<unsigned short>>> map_data; //entire data map | index: board < channel < samplevector
	map<int,vector<unsigned short>> map_meta; //entire meta map | index: board < metakey < value
	vector<uint64_t> vbuffer;
	map<int, vector<unsigned short>> map_acdcIF;
	vector<unsigned short> map_accIF;

	static void got_signal(int);
        void sendJCPLLSPIWord(unsigned int word, unsigned int boardMask = 0xff, bool verbose = false);
};

#endif
