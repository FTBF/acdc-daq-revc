#include "ACC.h" 
#include <cstdlib> 
#include <bitset> 
#include <sstream> 
#include <string> 
#include <thread> 
#include <algorithm> 
#include <thread> 
#include <fstream> 
#include <atomic> 
#include <signal.h> 
#include <unistd.h> 
#include <cstring>
#include <chrono> 
#include <iomanip>
#include <numeric>
#include <ctime>


using namespace std;

//sigint handling
std::atomic<bool> quitacc(false); //signal flag

void ACC::got_signal(int)
{
	quitacc.store(true);
}
//

ACC::ACC()
{
	usb = new stdUSB();
	if(!usb->isOpen())
	{
		writeErrorLog("Usb was unable to connect to ACC");
		delete usb;
		exit(EXIT_FAILURE);
	}

	emptyUsbLine();
}


ACC::~ACC()
{
	cout << "Calling acc destructor" << endl;
	clearAcdcs();
	emptyUsbLine();
	delete usb;
}

//sometimes it makes sense to repeatedly send
//a usb command until a response is sent back. 
//this function does that in a safe manner. 
vector<unsigned short> ACC::sendAndRead(unsigned int command, int buffsize)
{
	if(!checkUSB()) exit(EXIT_FAILURE);

	int send_counter = 0; //number of usb sends
	int max_sends = 10; //arbitrary. 
	bool loop_breaker = false; 
	
	vector<unsigned short> tempbuff;
	while(!loop_breaker)
	{

		usb->sendData(command);
		send_counter++;
		tempbuff = usb->safeReadData(buffsize + 2);

		//this is a BAD failure that I have seen happen
		//if the USB line is flooded by the ACC. The 
		//result is that some c++ memory has been overwritten
		//(calloc buffer doesn't allocate enough bytes). 
		//You need to continue with a crazy large buffsize to
		//clear the USB line or multiple things will crash. 
		if((int)tempbuff.size() > buffsize + 2)
		{
			buffsize = 10*buffsize; //big enough to hold a whole ACDC buffer
			continue;
		}
		//if the buffer is non-zero size, then
		//we got a message back. break the loop
		if(tempbuff.size() > 0)
		{
			loop_breaker = true;
		}
		if(send_counter == max_sends)
		{
			loop_breaker = true;
		}
	}
	return tempbuff;
}

//just read until data readout times out. 
//I have found that occassionally after a reboot
//of the boards or after a serious usb failure that
//the constant memory size buffers get over-filled and
//crash a raspberry pi or muddle the DAQ system. This 
//is an attempt to clear the line and have a fresh state. 
void ACC::emptyUsbLine()
{
	if(!checkUSB()) exit(EXIT_FAILURE);

	int send_counter = 0; //number of usb sends
	int max_sends = 10; //arbitrary. 
	bool loop_breaker = false; 
	unsigned int command = 0x00200000; // a refreshing command
	vector<unsigned short> tempbuff;
	while(!loop_breaker)
	{
		usb->sendData(command);
		send_counter++;
		tempbuff = usb->safeReadData(SAFE_BUFFERSIZE);

		//if it is exactly an ACC buffer size, success. 
		if(tempbuff.size() == 32)
		{
			loop_breaker = true;
		}
		if(send_counter > max_sends)
		{
			string err_msg = "Something wrong with USB line, waking it up. got ";
			err_msg += to_string(tempbuff.size());
			err_msg += " words";
			writeErrorLog(err_msg);
			usbWakeup();
			tempbuff = usb->safeReadData(SAFE_BUFFERSIZE);
			if(tempbuff.size() == 32){
				writeErrorLog("Usb woke up. Problem is fixed");
			}else{
				writeErrorLog("Usb still sleeping. Problem is not fixed.");
 			}
			loop_breaker = true;
		}
	}
}

bool ACC::checkUSB()
{
	if(!usb->isOpen())
	{
		bool retval = usb->createHandles();
		if(!retval)
		{
			writeErrorLog("Cannot connect to ACC usb");
			return false;
		}
	}else{
		return true;
	}
	return true;
}


//function that returns a pointer
//to the private ACC usb line. 
//currently used by the Config class
//to set configurations. 
stdUSB* ACC::getUsbStream()
{
	return usb;
}

//reads ACC info buffer only. short buffer
//that does not rely on any ACDCs to be connected. 
vector<unsigned short> ACC::readAccBuffer()
{
	if(!checkUSB()) exit(EXIT_FAILURE);

	//writing this tells the ACC to respond
	//with its own metadata
	unsigned int command = 0x00200000; 
	//is OK to just pound the ACC with the
	//command until it responds. in a loop function "sendAndRead"
	vector<unsigned short> v_buffer = sendAndRead(command, SAFE_BUFFERSIZE/4);
	if(v_buffer.size() == 0)
	{
		writeErrorLog("USB comms to ACC are broken");
		cout << "(1) Turn off the ACC" << endl;
		cout << "(2) Unplug the USB cable power" << endl;
		cout << "(3) Turn on ACC" << endl;
		cout << "(4) Plug in USB" << endl;
		cout << "(5) Wait for green ACC LED to turn off" << endl;
		cout << "(5) Repeat if needed" << endl;
		cout << "Trying USB reset before closing... " << endl;
		usb->reset();
		std::this_thread::sleep_for(chrono::seconds(1));
		exit(EXIT_FAILURE);
	}
	lastAccBuffer = v_buffer; //save as a private variable
	return v_buffer; //also return as an option
}

//this queries the Acc buffer,
//finds which ACDCs are connected, then
//creates the ACDC objects. In this particular
//function, the ACDC objects are then
//filled with metadata by querying for 
//ACDC buffer readout. This action also
//allows for filling of ACC and ACDC private
// variables. The ACC object keeps
//the following info:
//Which ACDCs are LVDS aligned?
//Are there events in ACC ram and which ACDCs?
//retval: 
//1 if there are ACDCs connected
//0 if none. 
int ACC::createAcdcs()
{
	//loads a ACC buffer into private member
	readAccBuffer(); 

	//parses the last acc buffer to 
	//see which ACDCs are aligned. 
	whichAcdcsConnected(); 

	//if there are no ACDCs, return 0
	if(alignedAcdcIndices.size() == 0)
	{
		writeErrorLog("No aligned Acdc indices");
		return 0;
	}
	
	//clear the acdc vector
	clearAcdcs();

	//create ACDC objects with their board numbers
	//loaded into alignedAcdcIndices in the
	//last function call. 
	for(int bi: alignedAcdcIndices)
	{
		cout << "Creating a new ACDC object for detected ACDC: " << bi << endl;
		ACDC* temp = new ACDC();
		temp->setBoardIndex(bi);
		acdcs.push_back(temp);
	}
	parsePedsAndConversions(); //load pedestals and LUT conversion factors onto ACDC objects.
	return 1;
}

//If the pedestals were set, and an ADC-counts to mV lineary scan
//was made for these boards, then there are autogenerated calibration
//files that should be loaded into private variables of the ACDC. 
//They are save to some directory as text files and stored for long-term
//use because the actual pedestal values and ADC-to-mv conversion
//for each sample will persist for longer than a single script function call.
//The directory and filenames are defines in the ACC.h. This function
//returns the number of board indices that do not have associated LIN
//files in the calibration directory. 
int ACC::parsePedsAndConversions()
{
	int linCounter = 0; //counts how many board numbers DO NOT have matching lin files
	int pedCounter = 0; //counts how many board numbers DO NOT have matching ped files
	double defaultConversion = 1200.0/4096.0; //default for ADC-to-mv conversion
	double defaultPed = 0.0; //sets the baseline around 600mV
	map<int, vector<double>> tempMap; 
	map<int, vector<double>> tempMap2; 
	string pedfilename;
	ifstream ifped;

	//loop over all connected boards
	for(ACDC* a: acdcs)
	{
		for(int bi: alignedAcdcIndices)
		{
			if(bi==a->getBoardIndex())
			{
				//look for the PED and LIN files for this index
				pedfilename = string(CALIBRATION_DIRECTORY) + string(PED_TAG)  + "_board" + to_string(bi) + ".txt";
				ifped.open(pedfilename, ios_base::in);

				//if the associated file does not exist
				//set default values defined above. 
				if(!(bool)ifped)
				{
					string err_msg = "WARNING: Your boards do not have a pedestal calibration in the calibration folder: ";  
					err_msg += CALIBRATION_DIRECTORY;
					writeErrorLog(err_msg);
					pedCounter++;

					for(int channel = 0; channel < a->getNumCh(); channel++)
					{
						for(int smp=0; smp<NUM_SAMP; smp++)
						{
							tempMap[channel].push_back(defaultPed);
						}
					}

					a->setPeds(tempMap, bi);	 
				}else //otherwise, parse the file.
				{
					a->readPedsFromFile(ifped, bi);
				}
			}
		}
		ifped.close();

		string linfilename = string(CALIBRATION_DIRECTORY) + string(LIN_TAG)  + ".txt";
		ifstream iflin(linfilename);
		if(!(bool)iflin)
		{
			string err_msg = "WARNING: Your boards do not have a linearity scan calibration in the calibration folder: ";  
			err_msg += CALIBRATION_DIRECTORY;
			writeErrorLog(err_msg);
			linCounter++;
			for(int channel = 1; channel <= a->getNumCh(); channel++)
			{
				vector<double> vtemp(a->getNumSamp(), defaultConversion); //vector of defaults
				tempMap2[channel] = vtemp;
			}
			a->setConv(tempMap2);
		}
		else //otherwise, parse the file. 
		{
			a->readConvsFromFile(iflin);
		}

		iflin.close();
	}
	return linCounter;
}

//properly delete Acdc vector
//as each one was created by new. 
void ACC::clearAcdcs()
{
	for(int i = 0; i < (int)acdcs.size(); i++)
	{
		delete acdcs[i];
	}
	acdcs.clear();
}



void ACC::printByte(unsigned short val, int format)
{	
	stringstream ss;
	ss << std::hex << val;   
	unsigned n;

	if(format == 1)//decimal	
	{
		cout << val; 
	}else if(format == 2)//hex
	{       
		string hexstr(ss.str());
		cout << hexstr; 
	}else if(format == 3)//binary
	{	
		ss >> n;
		bitset<16> b(n);
		cout << b.to_string(); 
	}
}

vector<int> ACC::whichAcdcsConnected()
{
	//New sequence to ask the ACC to reply with the number of boards connected 
	enableTransfer(0); //Disables the PSEC4 frame data transfer for this sequence. Has to be set to HIGH later again

	usleep(100000);

	unsigned int command = 0x000200FF; //Resets the RX buffer on all 8 ACDC boards
	usb->sendData(command);
	command = 0x00030000; //Sends a reset for detected ACDC boards to the ACC
	usb->sendData(command);
	command = 0xFFD00000; //Request a 32 word ACDC ID frame containing all important infomations
	usb->sendData(command);

	usleep(100000);

	readAccBuffer(); //represents 0x00200000 and reads the requested ACDC ID frame  

	vector<int> connectedBoards;
	if(lastAccBuffer.size() != 32) //Check if buffer size is 32 words
	{
		string err_msg = "Something wrong with ACC buffer, size: ";  
		err_msg += to_string(lastAccBuffer.size());
		writeErrorLog(err_msg);
		return connectedBoards;
	}

	//for explanation of the "2", see INFO1 of CC_STATE in packetUSB.vhd
	unsigned short alignment_packet = lastAccBuffer.at(7); 
	//binary representation of this packet is 1 if the
	//board is connected for both the first two bytes
	//and last two bytes. 
	
	for(int i = 0; i < MAX_NUM_BOARDS; i++)
	{
		if(lastAccBuffer.at(16+i) != 32){
			cout << "Board "<< i << " not with 32 words after ACC buffer read ";
			cout << "Size " << lastAccBuffer.at(16+i)  << endl;
			continue;
		}

		//both (1<<i) and (1<<i+8) should be true if aligned & synced respectively
		if((alignment_packet & (1 << i)))
		{
			//the i'th board is connected
			connectedBoards.push_back(i);
		}
	}

	//this allows no vector clearing to be needed
	alignedAcdcIndices = connectedBoards;
	cout << "Connected Boards: " << connectedBoards.size() << endl;
	return connectedBoards;
}


//turns {0, 3, 6} into 0x00000049 (b1001001)
unsigned int ACC::vectorToUnsignedInt(vector<int> a)
{
	unsigned int result = 0x00000000;
	for(int val: a)
	{
		if(val < 32)
		{
			result = result | (1 << val);
		}
	}
	return result;
}

//turns {0, 3, 6} into 0x0049 (b1001001)
unsigned short ACC::vectorToUnsignedShort(vector<int> a)
{
	unsigned short result = 0x0000;
	for(int val: a)
	{
		if(val < 16)
		{
			result = result | (1 << val);
		}
	}
	return result;
}

vector<int> ACC::unsignedShortToVector(unsigned short a)
{
	vector<int> result;
	int lenOfShort = 16;
	for(int i = 0; i < lenOfShort; i++)
	{
		if(a & (1 << i))
		{
			result.push_back(i);
		}
	}
	return result;
}

//sends software trigger to all connected boards. 
//bin option allows one to force a particular 160MHz
//clock cycle to trigger on. anything greater than 3
//is defaulted to 0. 
void ACC::setSoftwareTrigger(vector<int> boards)
{
	
	//default value for "boards" is empty. If so, then
	//software trigger all active boards from last
	//buffer query. 
	if(boards.size() == 0)
	{
		boards = alignedAcdcIndices;
	}

	//Prepare Software trigger
	unsigned int command = 0x00300000; //Turn trigger to OFF on the acc
	usb->sendData(command);
	command = 0xFFB00000; //Turn the trigger OFF on all ACDCs
	usb->sendData(command);

	command = 0x003100FF;
	usb->sendData(command);

	//Set the trigger
	command = 0xFFB00001; //Sets the trigger of all ACDC boards to 1 = Software trigger
	usb->sendData(command);
	command = 0x00300FF1; //Sets all ACDC boards to software trigger on the ACC 
	usb->sendData(command);
}

void ACC::softwareTrigger()
{
	//Software trigger
	unsigned int command = 0x00100000;
	usb->sendData(command);
}


//checks to see if there are any ACDC buffers
//in the ram of the ACC. If waitForAll = true (false by default),
//it will continue checking until all alignedAcdcs have sent
//data to the ACC RAM. Unfortunately, the CC event number doesnt
//increment in hardware trigger mode, so the evno is sent in 
//explicitly to keep data files consistent. 
//"raw" will subtract ped and convert from LUT calibration
//if set to false. 
//return codes:
//0 = data found and parsed successfully
//1 = data found but had a corrupt buffer
//2 = no data found
int ACC::readAcdcBuffers(bool raw, string timestamp, int oscopeOnOff)
{
	//First, loop and look for 
	//a fullRam flag on ACC indicating
	//that ACDCs have sent data to the ACC
	vector<int> boardsReadyForRead;
	unsigned int command;

	//filename logistics
	string outfilename = "./Results/";
	string datafn;
	ofstream dataofs;

    enableTransfer(0);
    for(int bi: alignedAcdcIndices)
    {
		command = 0x00210000;
		command = command | bi;
		usb->sendData(command);
	}
    enableTransfer(1);
    usleep(6000);

    int maxCounter=0;
	while(true)
	{
		command = 0x00200000;
		usb->sendData(command);

		lastAccBuffer = usb->safeReadData(32);
		
		if(lastAccBuffer.size()==0)
		{
			maxCounter++;
			continue;
		}

		for(int k=0; k<MAX_NUM_BOARDS; k++)
		{
			if(lastAccBuffer.at(16+k)==7795)
			{
				boardsReadyForRead.push_back(k);
			}
		}
		if(boardsReadyForRead.size()>0)
		{
			break;
		}
		maxCounter++;
		if(maxCounter>15)
		{
			return 2;
		}
	}

	//enableTransfer(1);

	//----corrupt buffer checks begin
	//sometimes the ACDCs dont send back good
	//data. It is unclear why, but we would
	//just rather throw this event away. At
	//the end, the function will return based
	//on whether it finds any corrupt buffers.
	//each ACDC needs to be queried individually
	//by the ACC for its buffer. 
	for(int bi: boardsReadyForRead)
	{
		unsigned int command = 0x00210000; //base command for set readmode
		command = command | (unsigned int)(bi); //which board to read
		usb->sendData(command);
		usleep(6000);
		//read only once. sometimes the buffer comes up empty. 
		//made a choice not to pound it with a loop until it
		//responds. 
		vector<unsigned short> acdc_buffer = usb->safeReadData(7795);
		if(acdc_buffer.size() !=  7795 && acdc_buffer.size()!=7827){
			string err_msg = "Couldn't read 7795 words as expected! Tryingto fix it! Size was: ";
			err_msg += to_string(acdc_buffer.size());
			writeErrorLog(err_msg);
		}

		if(acdc_buffer.size() == 7827)
		{
			auto first = acdc_buffer.cbegin() + 32;
			auto last = acdc_buffer.cbegin() + 7827;

			vector<unsigned short> tempVec(first, last);
			acdc_buffer.clear();
			acdc_buffer = tempVec;
		}

		bool corruptBuffer = false;
		if(acdc_buffer.size() == 0)
		{
			corruptBuffer = true;
		}

		if(corruptBuffer)
		{
			writeErrorLog("Early corrupt buffer");
			return 1;
		}

		//save this buffer a private member of ACDC
		//by looping through our acdc vector
		//and checking each index 
		for(ACDC* a: acdcs)
		{
			if(a->getBoardIndex() == bi)
			{
				meta.checkAndInsert("Board", bi);
				//tells it explicitly to load the data
				//component of the buffer into private memory. 
				int retval;
				/*
				string rawfn = outfilename + "Raw_b" + to_string(bi) + ".txt";
				ofstream rawofs(rawfn.c_str(), ios::app | ios::binary); //trunc overwrites
				a->writeRawDataToFile(acdc_buffer, rawofs);
				*/
				retval = a->parseDataFromBuffer(acdc_buffer, oscopeOnOff, bi); 
				corruptBuffer = meta.parseBuffer(acdc_buffer);
				if(corruptBuffer)
				{
					writeErrorLog("Metadata error not parsed correctly");
					return 1;
				}
				map_meta[bi] = meta.getMetadata();
				if(retval !=0)
				{
					string err_msg = "Corrupt buffer caught at PSEC data level (2)";
					if(retval == 3)
					{
						err_msg += "Because of the Metadata buffer";
					}
					writeErrorLog(err_msg);
					corruptBuffer = true;

					a->writeRawBufferToFile(acdc_buffer);	
				}

				if(corruptBuffer)
				{
					string err_msg = "got a corrupt buffer with retval ";
					err_msg += to_string(retval);
					writeErrorLog(err_msg);
					return 1;
				}
				
				//filename logistics
				if(oscopeOnOff==0)
				{
					datafn = outfilename + "Data_" + timestamp + ".bin";
				}else if(oscopeOnOff==1)
				{
					datafn = outfilename + "Data_Oscope_b" + to_string(bi) + ".txt";
					dataofs.open(datafn.c_str(), ios_base::trunc); //trunc overwrites
					a->writeDataForOscope(dataofs);
				}
				map_data[bi] = a->returnData();
				
			}
		}
	}
	if(oscopeOnOff==0)
	{
		dataofs.open(datafn.c_str(), ios::app | ios::binary); //trunc overwrites
		writePsecData(dataofs, boardsReadyForRead);
	}
	return 0;
}


//identical to readAcdcBuffer but does an infinite
//loop when the trigMode is 1 (hardware trig) and
//switches toggles waitForAll depending on trig mode. 
//Unfortunately, the CC event number doesn't increment
//in hardware trigger mode, so that needs to be sent
//in explicitly via the logData function. 
//"raw" will subtract ped and convert from LUT calibration
//if set to false. 
//0 = data found and parsed successfully
//1 = data found but had a corrupt buffer
//2 = no data found
int ACC::listenForAcdcData(int trigMode, bool raw, string timestamp, int oscopeOnOff)
{
	bool waitForAll = false;
	vector<int> boardsReadyForRead; //list of board indices that are ready to be read-out
	unsigned int command;
	//filename logistics
	string outfilename = "./Results/";
	string datafn;
	ofstream dataofs;

	//this function is simply readAcdcBuffers
	//if the trigMode is software
	if(trigMode == 1)
	{
		int retval;
		//The ACC already sent a trigger, so
		//tell it not to send another during readout. 
		retval = readAcdcBuffers(raw, timestamp, oscopeOnOff);
		return retval;
	}

	//duration variables
	auto start = chrono::steady_clock::now(); //start of the current event listening. 
	auto now = chrono::steady_clock::now(); //just for initialization 
	auto printDuration = chrono::seconds(1); //prints as it loops and listens
	auto lastPrint = chrono::steady_clock::now();
	auto timeoutDuration = chrono::seconds(8); // will exit and reinitialize


	//setup a sigint capturer to safely
	//reset the boards if a ctrl-c signal is found
	struct sigaction sa;
    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = got_signal;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL);

    enableTransfer(0);
    for(int bi: alignedAcdcIndices)
    {
		command = 0x00210000;
		command = command | bi;
		usb->sendData(command);
	}
    enableTransfer(1);
    usleep(6000);

	try
	{
		while(true)
		{
			now = chrono::steady_clock::now();
			if(chrono::duration_cast<chrono::seconds>(now - lastPrint) > printDuration)
			{
				cout << "Have been waiting for a trigger for " << chrono::duration_cast<chrono::seconds>(now - start).count() << " seconds" << endl;
				lastPrint = chrono::steady_clock::now();
			}
			if(chrono::duration_cast<chrono::seconds>(now - start) > timeoutDuration)
			{
				return 2;
			}

			//if sigint happens, 
			//return value of 3 tells
			//logger what to do. 
			if(quitacc.load())
			{
				return 3;
			}

			command = 0x00200000;
			usb->sendData(command);

			lastAccBuffer = usb->safeReadData(32);
			
			if(lastAccBuffer.size()==0)
			{
				continue;
			}

			for(int k=0; k<MAX_NUM_BOARDS; k++)
			{
				if(lastAccBuffer.at(16+k)==7795)
				{
					boardsReadyForRead.push_back(k);
				}
			}
			if(boardsReadyForRead.size()>0)
			{
				break;
			}
		}
	
		//each ACDC needs to be queried individually
		//by the ACC for its buffer. 
		

		for(int bi: boardsReadyForRead)
		{
			//cout << "Read for board " << bi << endl;
			unsigned int command = 0x00210000; //base command for set readmode
			command = command | (unsigned int)(bi); //which board to read
			usb->sendData(command);
			usleep(6000);
			//read only once. sometimes the buffer comes up empty. 
			//made a choice not to pound it with a loop until it
			//responds. 
			vector<unsigned short> acdc_buffer = usb->safeReadData(7795);

			if(acdc_buffer.size()!=7795 && acdc_buffer.size()!=7827)
			{
				string err_msg = "Couldn't read 7795 words as expected! Tryingto fix it! Size was: ";
				err_msg += to_string(acdc_buffer.size());
				writeErrorLog(err_msg);
			}
			
			if(acdc_buffer.size() == 7827)
			{
				auto first = acdc_buffer.cbegin() + 32;
				auto last = acdc_buffer.cbegin() + 7827;

				vector<unsigned short> tempVec(first, last);
				acdc_buffer.clear();
				acdc_buffer = tempVec;
			}
			//----corrupt buffer checks begin
			//sometimes the ACDCs dont send back good
			//data. It is unclear why, but we would
			//just rather throw this event away. 
			bool corruptBuffer = false;
			if(acdc_buffer.size() == 0)
			{
				writeErrorLog("Empty Psec buffer read!");
				corruptBuffer = true;
			}

			if(corruptBuffer)
			{
				writeErrorLog("Early corrupt buffer");
				return 1;
			}

			//save this buffer a private member of ACDC
			//by looping through our acdc vector
			//and checking each index 
			for(ACDC* a: acdcs)
			{
				if(a->getBoardIndex() == bi)
				{
					meta.checkAndInsert("Board", bi); 

					//tells it explicitly to load the data
					//component of the buffer into private memory. 
					int retval;

					string rawfn = outfilename + "Raw_b" + to_string(bi) + ".txt";
					//ofstream rawofs(rawfn.c_str(), ios_base::trunc); //trunc overwrites
					//a->writeRawDataToFile(acdc_buffer, rawofs);
					retval = a->parseDataFromBuffer(acdc_buffer, oscopeOnOff, bi); 
					corruptBuffer = meta.parseBuffer(acdc_buffer);
					if(corruptBuffer)
					{
						writeErrorLog("Metadata error not parsed correctly");
						return 1;
					}
					map_meta[bi] = meta.getMetadata();

					if(retval !=0)
					{
						string err_msg = "Corrupt buffer caught at PSEC data level (2)";
						if(retval == 3)
						{
							err_msg += "Because of the Metadata buffer";
						}
						writeErrorLog(err_msg);
						corruptBuffer = true;

						a->writeRawBufferToFile(acdc_buffer);	
					}

					if(corruptBuffer)
					{
						string err_msg = "got a corrupt buffer with retval ";
						err_msg += to_string(retval);
						writeErrorLog(err_msg);
						return 1;
					}

					//filename logistics
					if(oscopeOnOff==0)
					{
						datafn = outfilename + "Data_" + timestamp + ".bin";
					}else if(oscopeOnOff==1)
					{
						datafn = outfilename + "Data_Oscope_b" + to_string(bi) + ".txt";
						dataofs.open(datafn.c_str(), ios_base::trunc); //trunc overwrites
						a->writeDataForOscope(dataofs);
					}
					map_data[bi] = a->returnData();
				}
			}
		}
		if(oscopeOnOff==0)
		{
			dataofs.open(datafn.c_str(), ios::app | ios::binary); 
			writePsecData(dataofs, boardsReadyForRead);
		}	
	}
	catch(string mechanism)
	{
		cout << mechanism << endl;
		return 2;
	}	
	return 0;
}


void ACC::enableTransfer(int onoff)
{
	if(onoff == 0)
	{ //Disable frame transfer
		unsigned int command = 0xFFB54000;
		usb->sendData(command);
	}else if(onoff == 1)
	{
		unsigned int command = 0xFFB50000;
		usb->sendData(command);
	}

}

//this is a function that sends a specific set
//of usb commands to configure the boards for
//triggering on real events. 
int ACC::initializeForDataReadout(int trigMode, unsigned int boardMask, int calibMode)
{
	// Creates ACDCs for readout
	createAcdcs();

	// Toogels the calibration mode on if requested
	toggleCal(calibMode,0xFFFF);

	unsigned int command;
	// Set trigger conditions
	switch(trigMode)
	{ 	
		case 0: //OFF
			cout << "Trigger is turned off" << endl;
			break;
		case 1: //Software trigger
			setSoftwareTrigger();
			break;
		case 2: //SMA trigger ACC 
			setHardwareTrigSrc(trigMode,boardMask);

			command = 0xFFB30000;
			command = (command & (command | (boardMask << 24))) | invertMode;
			usb->sendData(command);
			command = 0xFFB31000;
			command = (command & (command | (boardMask << 24))) | detectionMode;
			usb->sendData(command);
			break;
		case 3: //SMA trigger ACDC 
			setHardwareTrigSrc(trigMode,boardMask);

			command = 0xFFB20000;
			command = (command & (command | (boardMask << 24))) | invertMode;
			usb->sendData(command);

			command = 0xFFB21000;
			command = (command & (command | (boardMask << 24))) | detectionMode;
			usb->sendData(command);
			break;
		case 4: //Self trigger
			setHardwareTrigSrc(trigMode,boardMask);

			command = 0xFFB10000;
			for(int masknum=0; masknum<5; masknum++)
			{
				command = (command & (command | (boardMask << 24))) | (masknum << 12) | 0xFF;
				usb->sendData(command); 
			}

			command = 0xFFB15000;
			command = command | ChCoin;
			usb->sendData(command);

			command = 0xFFB16000;
			command = command | invertMode;
			usb->sendData(command);		

			command = 0xFFB17000;
			command = command | detectionMode;
			usb->sendData(command);		

			command = 0xFFB18000;
			command = command | enableCoin;
			usb->sendData(command);

			command = 0xFFA60000;
			command = command | (0x1F << 12) | threshold;
			usb->sendData(command);

			break;				
		case 5: //Self trigger with SMA validation on ACC
 			setHardwareTrigSrc(trigMode,boardMask);

			command = 0xFFB31001;
			usb->sendData(command);

			command = 0xFFB40000;
			usb->sendData(command);
			command = 0xFFB41280;
			usb->sendData(command);

			goto selfsetup;
			break;
		case 6: //Self trigger with SMA validation on ACDC
			setHardwareTrigSrc(trigMode,boardMask);

			command = 0xFFB31001;
			usb->sendData(command);

			command = 0xFFB40000;
			usb->sendData(command);
			command = 0xFFB41280;
			usb->sendData(command);

			goto selfsetup;
			break;
		case 7:
			setHardwareTrigSrc(trigMode,boardMask);

			command = 0xFFB40000;
			usb->sendData(command);
			command = 0xFFB41280;
			usb->sendData(command);

			command = 0xFFB20000;
			command = (command & (command | (boardMask << 24))) | invertMode;
			usb->sendData(command);
			command = 0xFFB21000;
			command = (command & (command | (boardMask << 24))) | detectionMode;
			usb->sendData(command);

			command = 0xFFB30000;
			command = (command & (command | (boardMask << 24))) | invertMode;
			usb->sendData(command);
			command = 0xFFB31000;
			command = (command & (command | (boardMask << 24))) | detectionMode;
			usb->sendData(command);
			break;
		case 8:
			setHardwareTrigSrc(trigMode,boardMask);

			command = 0xFFB40000;
			usb->sendData(command);
			command = 0xFFB41280;
			usb->sendData(command);

			command = 0xFFB20000;
			command = (command & (command | (boardMask << 24))) | invertMode;
			usb->sendData(command);
			command = 0xFFB21000;
			command = (command & (command | (boardMask << 24))) | detectionMode;
			usb->sendData(command);

			command = 0xFFB30000;
			command = (command & (command | (boardMask << 24))) | invertMode;
			usb->sendData(command);
			command = 0xFFB31000;
			command = (command & (command | (boardMask << 24))) | detectionMode;
			usb->sendData(command);
			break;
		default: // ERROR case
			writeErrorLog("Specified trigger is not known!");
			break;
		selfsetup:
 			command = 0xFFB10000;
			for(int masknum=0; masknum<5; masknum++)
			{
				command = (command & (command | (boardMask << 24))) | (masknum << 12) | 0xFF;
				usb->sendData(command); 
			}

			command = 0xFFB15000;
			command = command | ChCoin;
			usb->sendData(command);

			command = 0xFFB16000;
			command = command | invertMode;
			usb->sendData(command);		

			command = 0xFFB17000;
			command = command | detectionMode;
			usb->sendData(command);		

			command = 0xFFB18000;
			command = command | enableCoin;
			usb->sendData(command);

			command = 0xFFA60000;
			command = command | (0x1F << 12) | threshold;
			usb->sendData(command);
	}

	return 0;
}

//tells ACDCs to clear their ram. 
//necessary when closing the program, for example.
void ACC::dumpData()
{
		unsigned int command = 0xFFB51000; //base command for set readmode
		command = command;

		//send and read. 
		usb->sendData(command);
		usb->safeReadData(ACDC_BUFFERSIZE + 2);
}


//just tells each ACDC that has a fullRam flag
// to write its data and metadata maps to the filestreams. 
//Event number is passed to the ACDC objects to help. 
void ACC::writeEDataToFile(vector<unsigned short> acdc_buffer)
{
    string fnnn = "empty-acdc-buffer.txt";
    cout << "Printing empty ACDC buffer to file : " << fnnn << endl;
    ofstream ofs(fnnn);
    for(unsigned short k: acdc_buffer)
    {
        ofs << k << ", "; //decimal
	    stringstream ss;
	    ss << std::hex << k;
	    string hexstr(ss.str());
	    ofs << hexstr << ", "; //hex
	    unsigned n;
	    ss >> n;
	    bitset<16> b(n);
	    ofs << b.to_string(); //binary
        ofs << endl;
    }
    ofs.close();
}


//short circuits the Config - class based
//pedestal setting procedure. This is primarily
//used for calibration functions. Ped is in ADC counts
//from 0 to 4096. If boards is empty, will do to all connected
bool ACC::setPedestals(unsigned int ped, vector<int> boards)
{
	//default is empty vector, so do ped setting to all boards
	if(boards.size() == 0)
	{
		boards = alignedAcdcIndices;
	}

	//loop over connected boards
	int bi;
	int numChips;
	unsigned int command, chipAddress;
	int retval;
	for(ACDC* a: acdcs)
	{
		bi = a->getBoardIndex();
		//if this isn't in the selected board list, 
		//dont try to set a pedestal. 
		if(std::find(boards.begin(), boards.end(), bi) == boards.end())
		{
			continue;
		}

		numChips = a->getNumPsec();
		//set pedestals and thresholds
		for(int chip = 0; chip < numChips; chip++)
		{
			chipAddress = (1 << chip); //20 magic number

			//pedestal
			command = 0xFFA20000; //ped setting command. 
			command = command | (chipAddress << 12) | ped;
			retval = usb->sendData(command); //set ped on that chip
			if(retval != 0)
			{
				string err_msg = "Failed setting pedestal on board ";
				err_msg += to_string(bi);
				err_msg += " chip ";
				err_msg += to_string(chip);
				writeErrorLog(err_msg);
				return false;
			}
		}
	}

	
	return true;
}

//toggles the calibration input line switches that the
//ACDCs have on board. 0xFF selects all boards for this toggle. 
//Similar for channel mask, except channels are ganged in pairs of
//2 for hardware reasons. So 0x0001 is channels 1 and 2 enabled. 
void ACC::toggleCal(int onoff, unsigned int channelmask)
{
		//Prepare Software trigger
	unsigned int command = 0x00300000; //Turn trigger to OFF on the acc
	usb->sendData(command);
	command = 0xFFB00000; //Turn the trigger OFF on all ACDCs
	usb->sendData(command);

	command = 0xFFC00000;
	//the firmware just uses the channel mask to toggle
	//switch lines. So if the cal is off, all channel lines
	//are set to be off. Else, uses channel mask
	if(onoff == 1)
	{
		//channelmas is defualt 0x7FFF
		command = command | channelmask;
	}

	usb->sendData(command);
}

//there is an option to use one of
//the front end boards (ACDCs) as
//a hardware trigger source. It is
//set via the 1e0c register (somewhat bad
//design i think). 
//int src:
//0=ext, 3 = board0, 4=b1, 6=b2, 7=b3
void ACC::setHardwareTrigSrc(int src, unsigned int boardMask)
{
	if(src > 9){
		string err_msg = "Source: ";
		err_msg += to_string(src);
		err_msg += " will cause an error for setting Hardware triggers. Source has to be <9";
		writeErrorLog(err_msg);
	}

	//ACC hardware trigger
	unsigned int command = 0x00300FF0;
	command = (command & (command | (boardMask << 4))) | (unsigned short)src;
	usb->sendData(command);
	//ACDC hardware trigger
	command = 0xFFB00000;
	command = (command & (command | (boardMask << 24))) | (unsigned short)src;
	usb->sendData(command);
}

void ACC::writeErrorLog(string errorMsg)
{
    string err = "errorlog.txt";
    cout << "------------------------------------------------------------" << endl;
    cout << errorMsg << endl;
    cout << "------------------------------------------------------------" << endl;
    ofstream os_err(err, ios_base::app);
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%m-%d-%Y %X");
    os_err << "------------------------------------------------------------" << endl;
    os_err << ss.str() << endl;
    os_err << errorMsg << endl;
    os_err << "------------------------------------------------------------" << endl;
    os_err.close();
}

void ACC::writePsecData(ofstream& d, vector<int> boardsReadyForRead)
{
	vector<string> keys;
	vector<string> lines;
	map<string, unsigned short> extra_key;
	for(int bi: boardsReadyForRead)
	{
		extra_key = map_meta[bi];
		if(extra_key.size()!=0)
		{
			break;
		}
	}

	for(auto const& element : extra_key) {
		keys.push_back(element.first);
	}

	string delim = " ";
	string line;
	for(int enm=0; enm<NUM_SAMP; enm++)
	{
		line += to_string(enm);
		line += delim;
		for(int bi: boardsReadyForRead)
		{
			for(int ch=0; ch<NUM_CH; ch++)
			{
				line += to_string(map_data[bi][ch+1][enm]);
				line += delim;
			}
			if(enm<(int)keys.size())
			{
				line += to_string(map_meta[bi][keys[enm]]);
				line += delim;
			}else
			{
				line += to_string(0);
				line += delim;
			}
		}
		lines.push_back(line);
	}
	d.write((char*)&lines[0], lines.size() * sizeof(lines));
	d.close();
	/*
	for(int enm=0; enm<NUM_SAMP; enm++)
	{
		d << dec << enm << delim;
		for(int bi: boardsReadyForRead)
		{
			if(map_data[bi].size()==0)
			{
				cout << "Mapdata is empty" << endl;
				writeErrorLog("Mapdata empty");
			}
			for(int ch=0; ch<NUM_CH; ch++)
			{
				if(enm==0)
				{
					//cout << "Writing board " << bi << " and ch " << ch << ": " << map_data[bi][ch+1][enm] << endl;
				}
				d << map_data[bi][ch+1][enm] << delim;
			}
			if(enm<(int)keys.size())
			{
				d << hex << map_meta[bi][keys[enm]] << delim;

			}else
			{
				d << 0 << delim;
			}
		}
		d << endl;
	}
	d.close();
	*/
    string kl = "keylist.txt";
    ofstream ofs(kl);
    for(string k: keys)
    {
        ofs << k << endl; //decimal
    }
    ofs.close();
}

//------------seperate functions

//the lightest reset. does not
//try to realign LVDS, does not try
//to reset ACDCs, just tries to wake the
//USB chip. 
void ACC::usbWakeup()
{
	unsigned int command = 0x00200000;
	usb->sendData(command);
}

//This is sent down to the ACDCs
//and has them individually reset their
//timestamps, dll, self-trigger, etc. 
void ACC::resetACDCs()
{
	unsigned int command = 0xFFFF0000;
	usb->sendData(command);
}


