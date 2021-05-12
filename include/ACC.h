#ifndef _ACC_H_INCLUDED
#define _ACC_H_INCLUDED

#include "ACDC.h" // load the ACDC class
#include "stdUSB.h" //load the usb class 


using namespace std;

#define SAFE_BUFFERSIZE 100000 //used in setup procedures to guarantee a proper readout 
#define NUM_CH 30 //maximum number of channels for one ACDC board
#define MAX_NUM_BOARDS 8 // maxiumum number of ACDC boards connectable to one ACC 
#define CALIBRATION_DIRECTORY "./autogenerated_calibrations/" //calibration folder if PED and LUT are recorded
#define PED_TAG "PEDS_ACDC" // PED tag
#define LIN_TAG "LUT_ACDC" // LUT tag

class ACC
{
public:
	ACC(); // constructor
	~ACC(); //deconstructor

	//----------local return functions
	vector<int> getAlignedIndices(){return alignedAcdcIndices;} //returns vector of aligned acdc indices
	string getCalDirectory(){return CALIBRATION_DIRECTORY;} //returns the calibration directory
	string getPedTag(){return PED_TAG;} //returns the file tag for the pedestal calibration
	string getLinTag(){return LIN_TAG;}	//returns the file tag for the lut calibration
	int getTriggermode(){return trigMode;} //returns the set trigger mode
	map<int, map<int, vector<double>>> returnData(){return map_data;} //returns the entire data map | index: board < channel < samplevector
	map<int, map<string, unsigned short>> returnMeta(){return map_meta;} //returns the entire meta map | index: board < metakey < value 

	//----------local set functions for board setup
	void setNumChCoin(unsigned int in){SELF_number_channel_coincidence = in;} //sets the number of channels needed for self trigger coincidence	
	void setEnableCoin(int in){SELF_coincidence_onoff = in;} //sets the enable coincidence requirement flag for the self trigger
	void setThreshold(unsigned int in){SELF_threshold = in;} //sets the threshold for the self trigger
	void setPsecChipMask(vector<unsigned int> in){SELF_psec_channel_mask = in;} //sets the PSEC chip mask to set individual chips for the self trigger 
	void setPsecChannelMask(vector<unsigned int> in){SELF_psec_chip_mask = in;} //sets the PSEC channel mask to set individual channels for the self trigger 
	void setValidationWindow(unsigned int in){validation_window=in;} //sets the validation window length for required trigger modes
	void setTriggermode(int in){trigMode = in;} //sets the overall triggermode
	bool setPedestals(unsigned int ped, vector<int> boards = {}); //sets pedestal value for connected boards to a defined value
	void setDetectionMode(int in, int source) //sets the detection mode (edge or level) for chosen source
	{
		if(source==2)
		{
			ACC_detection_mode = in;
		}else if(source==3)
		{
			ACDC_detection_mode = in;
		}else if(source==4)
		{
			SELF_detection_mode = in;
		}
	}	
	void setSign(int in, int source) //sets the sign (normal or inverted) for chosen source
	{
		if(source==0)
		{
			ACC_sign = in;
		}else if(source==1)
		{
			ACDC_sign = in;
		}else if(source==2)
		{
			SELF_sign = in;
		}
	}

	//----------functions requiered for the setup
	int initializeForDataReadout(int trigMode = 0,unsigned int boardMask = 0xFF, int calibMode = 0); //uses all the settings given to the software to initilize the boards with the following functions
	int createAcdcs(); //creates ACDC objects, explicitly querying both buffers
	void toggleCal(int onoff, unsigned int channelmask = 0x7FFF); //toggles calibration input switch on boards
	void setSoftwareTrigger(vector<int> boards = {}); //prepares software trigger and sets on acc/acdc
	void setHardwareTrigSrc(int src, unsigned int boardMask = 0xFF); //prepares fardware trigger and sets on acc/acdc

	//----------functions requiered for the readout
	void softwareTrigger(); //sends the software trigger
	void enableTransfer(int onoff=0); //enables or disables the transfer from acdc to acc 
	int readAcdcBuffers(bool raw = false, string timestamp ="invalidTime"); //special version of listenForAcdcData made for the software trigger. Does not listen for a trigger flag but expects a software trigger
	int listenForAcdcData(int trigMode, bool raw = false, string timestamp="invalidTime"); //fucnction to listen for trigger flags and reading data
	int whichAcdcsConnected(); //checks which ACDC boards are connected
  	//----------write functions
	void writeErrorLog(string errorMsg); //writes an errorlog with timestamps for debugging
	void writePsecData(ofstream& d, vector<int> boardsReadyForRead); //main write for the data map
	void writeRawDataToFile(vector<unsigned short> buffer, string rawfn); //main write for the raw data vector

	//----------general functions
	void usbWakeup(); //wakes the usb line up by asking for an info frame of the acc
	void resetACDC(); //resets the acdc boards 
	stdUSB* getUsbStream(); //returns the private usb object
	void emptyUsbLine(); //attempting to remove the crashes due to non-empty USB lines at startup
	void dumpData(); //tells ACDCs to clear their ram

private:
	//----------all neccessary classes
	stdUSB* usb; //calls the usb class for communication
	Metadata meta; //calls the metadata class for file write
	vector<ACDC*> acdcs; //a vector of active acdc boards. 

	//----------all neccessary global variables
	int ACC_detection_mode; //var: ACC detection mode (level or edge)
	int ACC_sign; //var: ACC sign (normal or inverted)
	int ACDC_detection_mode; //var: ACDC detection mode (level or edge)
	int ACDC_sign; //var: ACDC sign (normal or inverted)
	int SELF_detection_mode; //var: self trigger detection mode (level or edge)
	int SELF_sign; //var: self trigger sign (normal or inverted)
	int SELF_coincidence_onoff; //var: flag to enable self trigger coincidence
	int trigMode; //var: decides the triggermode
	unsigned int SELF_number_channel_coincidence; //var: number of channels required in coincidence for the self trigger
	unsigned int SELF_threshold; //var: threshold for the selftrigger
	unsigned int validation_window; //var: validation window for some triggermodes
	vector<unsigned short> lastAccBuffer; //most recently received ACC buffer
	vector<int> alignedAcdcIndices; //number relative to ACC index (RJ45 port) corresponds to the connected ACDC boards
	vector<unsigned int> SELF_psec_channel_mask; //var: PSEC channels active for self trigger
	vector<unsigned int> SELF_psec_chip_mask; //var: PSEC chips actove for self trigger
	map<int, map<int, vector<double>>> map_data; //entire data map | index: board < channel < samplevector
	map<int, map<string, unsigned short>> map_meta; //entire meta map | index: board < metakey < value

	//-----------private functions for the setup and initilization
	vector<unsigned short> readAccBuffer(); //reads the ACC once for setup 
	
	vector<unsigned short> sendAndRead(unsigned int command, int buffsize); //wakes the usb line, only called in constructor. 
	bool checkUSB(); //checking usb line and returning or couting appropriately.  
	void clearAcdcs(); //memory deallocation for acdc vector. 
	int parsePedsAndConversions(); //puts ped and LUT-scan data into ACDC objects

	static void got_signal(int);
};

#endif
