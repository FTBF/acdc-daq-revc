#ifndef _ACC_H_INCLUDED
#define _ACC_H_INCLUDED

#include "ACDC.h"
#include "stdUSB.h"

using namespace std;

#define CC_BUFFERSIZE 3000 //will only be 32, but if more, it will crash your system. Sometimes the ACDC buffer comes instead
#define ACDC_BUFFERSIZE 10000 //will only be 8001
#define MAX_NUM_BOARDS 8
#define CALIBRATION_DIRECTORY "autogenerated_calibrations/"
#define PED_TAG "PEDS_ACDC-index-"
#define LIN_TAG "LUT_ACDC-index-"

class ACC
{
public:
	ACC();
	~ACC();
	void softReconstructor(); //a "destructor" and "constructor" that's callable. 

	void testFunction();

	//----------parsing functions (no usb comms)
	void printAccMetadata(bool pullNew = false);
	void printRawAccBuffer(bool pullNew = false);
	void printAcdcInfo(bool verbose = false);
	vector<int> checkFullRamRegisters(bool pullNew = false);
	vector<int> checkDigitizingFlag(bool pullNew = false);
	vector<int> checkDcPktFlag(bool pullNew = false);
	unsigned int vectorToUnsignedInt(vector<int> a); //utility for going from list to 101011 bits. 
	unsigned short vectorToUnsignedShort(vector<int> a);
	vector<int> unsignedShortToVector(unsigned short a);
	int getAccEventNumber(bool pullNew = false); //gets the Acc event number
	void writeAcdcDataToFile(ofstream& d, ofstream& m); 
	vector<int> getAlignedIndices(){return alignedAcdcIndices;} //returns vector of aligned acdc indices
	string getCalDirectory(){return CALIBRATION_DIRECTORY;}
	string getPedTag(){return PED_TAG;}
	string getLinTag(){return LIN_TAG;}
	
	//-----------functions that involve usb comms
	//(see cpp declaration for more comments above functions)
	int createAcdcs(); //creates ACDC objects, explicitly querying both buffers
	void softwareTrigger(vector<int> boards = {}, int bin = 0); //sends soft trigger to specified boards
	void toggleCal(int onoff, unsigned int boardmask = 0xFF, unsigned int channelmask = 0xFFFF); //toggles calibration input switch on boards
	int readAcdcBuffers(bool waitForAll = false, int evno = 0, bool raw = false); //reads the acdc buffers
	int listenForAcdcData(int trigMode, int evno = 0, bool raw = false); //almost identical to readAcdcBuffers but intended for real data triggering
	void initializeForDataReadout(int trigMode = 0);
	void dataCollectionCleanup(int trigMode = 0); //a set of usb commands to reset boards after data logging
	void dumpData(); //tells ACDCs to clear their ram
	bool setPedestals(unsigned int ped, vector<int> boards = {});
  void emptyUsbLine(); //attempting to remove the crashes due to non-empty USB lines at startup.


	//-----short usb send functions. found
	//-----at the end of the cpp file. 
	void setAccTrigInvalid(); //b004
	void resetAccTrigger(); //b0001
	void setFreshReadmode(); //c0001
	void resetAcdcTrigger(); //c0010
	void setHardwareTrigSrc(int src); //c0000 | complicated code
	void prepSync(); //preps sync? need to read firmware to understand this
	void makeSync(); //make sync? need to read firmware to understand this
	void setAccTrigValid(); //b0006

	//--reset functions
	void usbWakeup(); //40EFF;
	void resetACDCs(); //4F000;
	void hardReset(); //reset ACDCs and realign, closest thing to power cycle
	void alignLVDS();
	stdUSB* getUsbStream(); //returns the private usb object

private:
	stdUSB* usb;
	vector<unsigned short> lastAccBuffer; //most recently received ACC buffer
	vector<int> alignedAcdcIndices; //number relative to ACC index (RJ45 port)
	vector<ACDC*> acdcs; //a vector of active acdc boards. 
	vector<int> fullRam; //which boards have finished sending all of their data to the ACC. 
	vector<int> digFlag; //which boards (board index) have a dig start flag (dont know what that means)
	vector<int> dcPkt; //which boards have started sending data to the ACC but have not yet filled the RAM. 


	//-----------functions that involve usb comms
	vector<unsigned short> readAccBuffer();


	//-----------parsing functions (no usb comms)
	vector<int> whichAcdcsConnected(bool pullNew = false);
	void printByte(unsigned short val);
	vector<unsigned short> sendAndRead(unsigned int command, int buffsize); //wakes the usb line, only called in constructor. 
	bool checkUSB(); //checking usb line and returning or couting appropriately.  
	void clearAcdcs(); //memory deallocation for acdc vector. 
	int parsePedsAndConversions(); //puts ped and LUT-scan data into ACDC objects

	static void got_signal(int);
};

#endif