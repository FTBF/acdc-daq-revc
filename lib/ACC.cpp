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

/*ID:3+4 sigint handling*/
std::atomic<bool> quitacc(false); 
void ACC::got_signal(int){quitacc.store(true);}

/*------------------------------------------------------------------------------------*/
/*--------------------------------Constructor/Deconstructor---------------------------*/

/*ID:5 Constructor*/
ACC::ACC() : eth("192.168.46.108", "2007"), eth_burst("192.168.46.108", "2008")
{
	bool clearCheck;
}

ACC::ACC(const std::string& ip) : eth(ip, "2007"), eth_burst(ip, "2008")
{
	bool clearCheck;
}

/*ID:6 Destructor*/
ACC::~ACC()
{
}


/*------------------------------------------------------------------------------------*/
/*---------------------------Setup functions for ACC/ACDC-----------------------------*/

void ACC::parseCfg(const YAML::Node& config)
{
    
    //initializeForDataReadout(int trigMode, unsigned int boardMask, int calibMode);    
}

/*ID:9 Create ACDC class instances for each connected ACDC board*/
int ACC::createAcdcs()
{
	//Check for connected ACDC boards
	int retval = whichAcdcsConnected(); 
	if(retval==-1)
	{
		std::cout << "Trying to reset ACDC boards" << std::endl;
		eth.send(0x100, 0xFFFF0000);
		usleep(10000);
		int retval = whichAcdcsConnected();
		if(retval==-1)
		{
			std::cout << "After ACDC reset no changes, still no boards found" << std::endl;
		}
	}

	//if there are no ACDCs, return 0
	if(alignedAcdcIndices.size() == 0)
	{
		writeErrorLog("No aligned ACDC indices");
		return 0;
	}
	
	//Clear the ACDC class vector if one exists
	clearAcdcs();

	//Create ACDC objects with their board numbers
	//loaded into alignedAcdcIndices in the last function call. 
	for(int bi: alignedAcdcIndices)
	{
		ACDC* temp = new ACDC();
		temp->setBoardIndex(bi);
		acdcs.push_back(temp);
	}
	if(acdcs.size()==0)
	{
		writeErrorLog("No ACDCs created even though there were boards found!");
		return 0;
	}

	return 1;
}

/*ID:11 Queries the ACC for information about connected ACDC boards*/
int ACC::whichAcdcsConnected()
{
	int retval=0;
	unsigned int command;
	vector<int> connectedBoards;

	//New sequence to ask the ACC to reply with the number of boards connected 
	//Disables the PSEC4 frame data transfer for this sequence. Has to be set to HIGH later again
	enableTransfer(0); 
	usleep(1000);

	//Resets the RX buffer on all 8 ACDC boards
	eth.send(0x0020, 0xff);

	//Request and read the ACC info buffer and pass it the the corresponding vector
        uint64_t accInfo = eth.recieve(0x1011);

	unsigned short alignment_packet = ~((unsigned short)accInfo);
	for(int i = 0; i < MAX_NUM_BOARDS; i++)
	{	
		//both (1<<i) and (1<<i+8) should be true if aligned & synced respectively
		if((alignment_packet & (1 << i)))
		{
			//the i'th board is connected
			connectedBoards.push_back(i);
		}
	}
	if(connectedBoards.size()==0 || retval==-1)
	{
		return -1;
	}

	//this allows no vector clearing to be needed
	alignedAcdcIndices = connectedBoards;
	cout << "Connected Boards: " << alignedAcdcIndices.size() << endl;
	return 1;
}

/*ID 17: Main init function that controls generalk setup as well as trigger settings*/
int ACC::initializeForDataReadout(int trigMode, unsigned int boardMask, int calibMode)
{
	unsigned int command;
	int retval;

	std::cout << "Received board mask: ";
	printf("0x%02x\n", boardMask);
	
	// Creates ACDCs for readout
	retval = createAcdcs();
	if(retval==0)
	{
            writeErrorLog("ACDCs could not be created");
	}

        //check ACDC PLL Settings
        // REVIEW ERRORS AND MESSAGES 
        for(int bi: alignedAcdcIndices)
	{
            // read ACDC info frame 
            eth.send(0x100, 0x00D00000 | (1 << (bi + 24)));

            std::vector<uint64_t> acdcInfo = eth.recieve_many(0x1200 + bi, 32, EthernetInterface::NO_ADDR_INC);
            if((acdcInfo[0] & 0xffff) != 0x1234)
            {
                std::cout << "ACDC" << bi << " has invalid info frame" << std::endl;
            }

            if(!(acdcInfo[6] & 0x4)) std::cout << "ACDC" << bi << " has unlocked ACC pll" << std::endl;
            if(!(acdcInfo[6] & 0x2)) std::cout << "ACDC" << bi << " has unlocked serial pll" << std::endl;
            if(!(acdcInfo[6] & 0x1)) std::cout << "ACDC" << bi << " has unlocked white rabbit pll" << std::endl;

            if(!(acdcInfo[6] & 0x8))
            {
                // external PLL must be unconfigured, attempt to configure them 
                configJCPLL();

                // reset the ACDC after configuring JCPLL
                resetACDC();
                usleep(5000);

                // check PLL bit again
                // read ACD info frame 
                eth.send(0x100, 0x00D00000 | (1 << (bi + 24)));

                acdcInfo = eth.recieve_many(0x1200 + bi, 32, EthernetInterface::NO_ADDR_INC);
                if((acdcInfo[0] & 0xffff) != 0x1234)
                {
                    std::cout << "ACDC" << bi << " has invalid info frame" << std::endl;
                }
                
                if(!(acdcInfo[6] & 0x8)) writeErrorLog("ACDC" + std::to_string(bi) + " has unlocked sys pll");
            }
	}


	//disable all triggers
	//ACC trigger
        for(unsigned int i = 0; i < 8; ++i) eth.send(0x0030+i, 0);
	//ACDC trigger
	command = 0xffB00000;
	eth.send(0x100, command);
        //disable data transmission
        enableTransfer(0); 

	// Toogels the calibration mode on if requested
	toggleCal(calibMode, 0x7FFF, boardMask);

	//train manchester links
	eth.send(0x0060, 0);
	usleep(250);

        //scan hs link phases and pick optimal phase
        scanLinkPhase(boardMask);

	// Set trigger conditions
	switch(trigMode)
	{ 	
		case 0: //OFF
			writeErrorLog("Trigger source turned off");	
			break;
		case 1: //Software trigger
                        setHardwareTrigSrc(trigMode,boardMask);
			break;
		case 2: //Self trigger
			setHardwareTrigSrc(trigMode,boardMask);
			goto selfsetup;
		case 3: //Self trigger with validation 
			setHardwareTrigSrc(trigMode,boardMask);
                        //timeout 
			command = 0x00B20000;
			command = (command | (boardMask << 24)) | 40;
			eth.send(0x100, command);
			break;
                case 4:
			setHardwareTrigSrc(trigMode,boardMask);
			break;
		case 5: //Self trigger with SMA validation on ACC
 			setHardwareTrigSrc(trigMode,boardMask);
                        eth.send(0x0038, ACC_sign);

                        eth.send(0x0039, validation_start);
			
                        eth.send(0x003a, validation_window);
			
                        //eth.send(0x003b, PPSBeamMultiplexer);
			
			goto selfsetup;
			break;
		default: // ERROR case
			writeErrorLog("Trigger source Error");	
			break;
		selfsetup:
 			command = 0x00B10000;
			
			printf("%s","Channel-Mask: ");
			for(unsigned int k: SELF_psec_channel_mask){printf("0x%02x\t",k);}
			printf("\n");
			
			if(5!=SELF_psec_channel_mask.size())
			{
				writeErrorLog("PSEC mask error");	
			}
			
			std::vector<unsigned int> CHIPMASK = {0x00000000,0x00001000,0x00002000,0x00003000,0x00004000};
			for(int i=0; i<(int)SELF_psec_channel_mask.size(); i++)
			{		
				command = 0x00B10000;
				command = (command | (boardMask << 24)) | CHIPMASK[i] | SELF_psec_channel_mask[i]; printf("Mask: 0x%08x\n",command);
				eth.send(0x100, command);
			}
			
			command = 0x00B16000;
			command = (command | (boardMask << 24)) | SELF_sign;
                        eth.send(0x100, command);
			command = 0x00B15000;
			command = (command | (boardMask << 24)) | SELF_number_channel_coincidence;
                        eth.send(0x100, command);
			command = 0x00B18000;
			command = (command | (boardMask << 24)) | SELF_coincidence_onoff;
                        eth.send(0x100, command);
                        if(SELF_thresholds.size() >= 30)
                        {
                            for(int iChip = 0; iChip < 5; ++iChip)
                            {
                                for(int iChan = 0; iChan < 6; ++iChan)
                                {
                                    command = 0x00A60000;
                                    command = ((command + (iChan << 16)) | (boardMask << 24)) | (iChip << 12) | SELF_thresholds[6*iChip + iChan];
                                    eth.send(0x100, command);
                                }
                            }
                        }
                        else
                        {
                            // thorw some form of error here
                        }
	}

        //flush data FIFOs
	dumpData(boardMask);

	//Enables the transfer of data from ACDC to ACC
	eth_burst.setBurstTarget();
	eth.setBurstMode(true);
	enableTransfer(3); 
        
	return 0;
}

/*ID 21: Set up the hardware trigger*/
void ACC::setHardwareTrigSrc(int src, unsigned int boardMask)
{
	if(src > 9){
		string err_msg = "Source: ";
		err_msg += to_string(src);
		err_msg += " will cause an error for setting Hardware triggers. Source has to be <9";
		writeErrorLog(err_msg);
	}

        int ACCtrigMode = 0;
        int ACDCtrigMode = 0;
        switch(src)
        {
        case 0:
            ACCtrigMode = 0;
            ACDCtrigMode = 0;
            break;
        case 1:
            ACCtrigMode = 1;
            ACDCtrigMode = 1;
            break;
        case 2:
            ACCtrigMode = 0;
            ACDCtrigMode = 2;
            break;
        case 3:
            ACCtrigMode = 2;
            ACDCtrigMode = 3;
            break;
        case 4:
            ACCtrigMode = 2;
            ACDCtrigMode = 1;
            break;
        default:
            ACCtrigMode = 0;
            ACDCtrigMode = 0;
            break;
        
        }

	//ACC hardware trigger
        for(unsigned int i = 0; i < 8; ++i)
        {
            if((boardMask >> i) & 1) eth.send(0x0030+i, ACCtrigMode);
            else                     eth.send(0x0030+i, 0);
        }
	//ACDC hardware trigger
	unsigned int command = 0x00B00000;
	command = (command | (boardMask << 24)) | (unsigned short)ACDCtrigMode;
	eth.send(0x100, command);
}

/*ID 20: Switch for the calibration input on the ACC*/
void ACC::toggleCal(int onoff, unsigned int channelmask, unsigned int boardMask)
{
	unsigned int command = 0x00C00000;
	//the firmware just uses the channel mask to toggle
	//switch lines. So if the cal is off, all channel lines
	//are set to be off. Else, uses channel mask
	if(onoff == 1)
	{
		//channelmas is default 0x7FFF
		command = (command | (boardMask << 24)) | channelmask;
	}else if(onoff == 0)
	{
		command = (command | (boardMask << 24));
	}
        eth.send(0x100, command);
}


/*------------------------------------------------------------------------------------*/
/*---------------------------Read functions listening for data------------------------*/

/*ID 14: Software read function*/
int ACC::readAcdcBuffers(bool raw, string timestamp)
{
	bool corruptBuffer;
	vector<int> boardsReadyForRead;
	map<int,int> readoutSize;
	unsigned int command;
	int maxCounter=0;
	bool clearCheck;

	//filename logistics
	string outfilename = "./Results/";
	string datafn;
	ofstream dataofs;

	//Enables the transfer of data from ACDC to ACC
   	//enableTransfer(1);
        //eth_burst.setBurstTarget();
        //eth.setBurstMode(true);

        
	std::cout << "Start looking for trigger conditions" << std::endl;
	while(true)
	{
		boardsReadyForRead.clear();
		readoutSize.clear();
		//Request the ACC info frame to check data FIFOs
                std::vector<uint64_t> fifoOcc = eth.recieve_many(0x1130, 8);
                
		for(int k=0; k<MAX_NUM_BOARDS; k++)
		{
                      if(fifoOcc[k]>=PSECFRAME)
                      {
                          boardsReadyForRead.push_back(k);
                          readoutSize[k] = PSECFRAME;
                      }
		}

		//old trigger
		if(boardsReadyForRead==alignedAcdcIndices)
		{
			break;
		}

		maxCounter++;
		if(maxCounter>5000)
		{
			std::cout << "Failed to find a trigger condition" << std::endl;
			return 2;
		}
	}
	std::cout << "Trigger condition found starting to read data" << std::endl;
	for(int bi: boardsReadyForRead)
	{
		std::cout << "Start reading board " << bi << std::endl;
		//base command for set readmode and which board bi to read
                eth.send(0x22, bi);

                std::vector<uint64_t> acdc_data = eth_burst.recieve_burst(1541);

		//Handles buffers =/= 7795 words
		if(acdc_data.size() != 1541)
		{
			string err_msg = "Couldn't read ";
			err_msg += to_string(1540);
			err_msg += " words as expected! Tryingto fix it! Size was: ";
			err_msg += to_string(acdc_data.size());
			writeErrorLog(err_msg);
			return 1;
		}
		
		//save this buffer a private member of ACDC
		//by looping through our acdc vector
		//and checking each index 
		for(ACDC* a: acdcs)
		{
			if(a->getBoardIndex() == bi)
			{
				int retval;

				//If raw data is requested save and return 0
				if(raw==true)
				{
                                    //vbuffer = acdc_data;
					string rawfn = outfilename + "Raw_" + timestamp + "_b" + to_string(bi) + ".txt";
					writeRawDataToFile(acdc_data, rawfn);
					break;
				}
                                else
				{
					retval = a->parseDataFromBuffer(acdc_data); 
					//corruptBuffer = meta.parseBuffer(acdc_buffer);
					//if(corruptBuffer)
					//{
					//	writeErrorLog("Metadata error not parsed correctly");
					//	return 1;
					//}
//					meta.checkAndInsert("Board", bi);
//					map_meta[bi] = meta.getMetadata();
					if(retval !=0)
					{
						string err_msg = "Corrupt buffer caught at PSEC data level (2)";
						if(retval == 3)
						{
							err_msg += "Because of the Metadata buffer";
						}
						writeErrorLog(err_msg);
						corruptBuffer = true;
					}

					if(corruptBuffer)
					{
						string err_msg = "got a corrupt buffer with retval ";
						err_msg += to_string(retval);
						writeErrorLog(err_msg);
						return 1;
					}
	
					map_data[bi] = a->returnData();
				}
			}
		}
	}
	std::cout << "Finished reading data for all boards" << std::endl;
	std::cout << "------------------------------------" << std::endl;
	if(raw==false && strcmp(timestamp.c_str(),"Oscope_b")!=0)
	{
		datafn = outfilename + "Data_" + timestamp + ".txt";
		dataofs.open(datafn.c_str(), ios::app);
		writePsecData(dataofs, boardsReadyForRead);
	}

        //eth.setBurstMode(false);

	return 0;
}

/*ID 15: Main listen fuction for data readout. Runs for 5s before retuning a negative*/
int ACC::listenForAcdcData(int trigMode, bool raw, const string& timestamp, const string& label)
{
    (void)trigMode; //currently unused

    bool corruptBuffer;
    vector<int> boardsReadyForRead;
    map<int,int> readoutSize;
    unsigned int command; 
    bool clearCheck;

    //filename logistics
    string outfilename = "./Results/";
    string datafn;
    ofstream dataofs;

    string rawfn;
    if(raw==true)
    {
        //vbuffer = acdc_data;
        rawfn = outfilename + "Raw_";
        if(label.size() > 0) rawfn += label + "_";
        rawfn += timestamp + "_b";
    }

    //this function is simply readAcdcBuffers
    //if the trigMode is software
    //if(trigMode == 1)
    //{
    //      int retval = readAcdcBuffers(raw, timestamp);
    //      return retval;
    //}

    //setup a sigint capturer to safely
    //reset the boards if a ctrl-c signal is found
    struct sigaction sa;
    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = got_signal;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL);

    //duration variables
    auto start = chrono::steady_clock::now(); //start of the current event listening. 
    auto now = chrono::steady_clock::now(); //just for initialization 
    auto printDuration = chrono::seconds(2); //prints as it loops and listens
    auto lastPrint = chrono::steady_clock::now();
    auto timeoutDuration = chrono::seconds(20); // will exit and reinitialize

    //Request the ACC info frame to check buffers
    while(true)
    {
        //Clear the boards read vector
        boardsReadyForRead.clear(); 
        readoutSize.clear();

        std::vector<uint64_t> fifoOcc = eth.recieve_many(0x1130, 8);

        //Time the listen fuction
        now = chrono::steady_clock::now();
        if(chrono::duration_cast<chrono::seconds>(now - lastPrint) > printDuration)
        {
            string err_msg = "Have been waiting for a trigger for ";
            err_msg += to_string(chrono::duration_cast<chrono::seconds>(now - start).count());
            err_msg += " seconds";
            writeErrorLog(err_msg);
            for(int i=0; i<MAX_NUM_BOARDS; i++)
            {
                string err_msg = "Buffer for board ";
                err_msg += to_string(i);
                err_msg += " has ";
                err_msg += to_string(fifoOcc[i]);
                err_msg += " words";
                writeErrorLog(err_msg);
            }
            lastPrint = chrono::steady_clock::now();
        }
        if(chrono::duration_cast<chrono::seconds>(now - start) > timeoutDuration)
        {
            return 2;
        }

        //If sigint happens, return value of 3
        if(quitacc.load())
        {
            return 3;
        }

        for(int k=0; k<MAX_NUM_BOARDS; k++)
        {
            if(fifoOcc[k]>=PSECFRAME)
            {
                boardsReadyForRead.push_back(k);
                readoutSize[k] = PSECFRAME;
            }
        }

        //old trigger
        if(boardsReadyForRead==alignedAcdcIndices)
        {
            break;
        }
    }

    //read out data FIFOs 
    for(int bi: boardsReadyForRead)
    {
        //base command for set data readmode and which board bi to read
        eth.send(0x22, bi);

        std::vector<uint64_t> acdc_data = eth_burst.recieve_burst(1541);

        //Handles buffers of incorrect size
        if((int)acdc_data.size() != 1541)
        {
            string err_msg = "Couldn't read " + std::to_string(readoutSize[bi]) + " words as expected! Tryingto fix it! Size was: ";
            err_msg += to_string(acdc_data.size());
            writeErrorLog(err_msg);
            return 1;
        }


        //save this buffer a private member of ACDC
        //by looping through our acdc vector
        //and checking each index 
        //FIX THIS NONSENSE
        for(ACDC* a: acdcs)
        {
            if(a->getBoardIndex() == bi)
            {
                int retval;

                //If raw data is requested save and return 0
                if(raw==true)
                {
                    writeRawDataToFile(acdc_data, rawfn + to_string(bi) + ".txt");
                    break;
                }
                else
                {
                    //parśe raw data to channel data and metadata
                    retval = a->parseDataFromBuffer(acdc_data);
                    map_data[bi] = a->returnData(); 
                    if(retval == -3)
                    {
                        break;
                    }
                    else if(retval == 0)
                    {
//                        if(metaSwitch == 1)
//                        {
//                            retval = meta.parseBuffer(acdc_buffer,bi);
//                            if(retval != 0)
//                            {
//                                writeErrorLog("Metadata error not parsed correctly");
//                                return 1;                                               
//                            }
//                            else
//                            {
//                                map_meta[bi] = meta.getMetadata();
//                            }
//                        }
//                        else
//                        {
//                            map_meta[bi] = {0};
//                        }
                    }
                    else
                    {
                        writeErrorLog("Data parsing went wrong");
                        return 1;
                    }                               
                }
            }
        }
    }
    if(raw==false)
    {
        datafn = outfilename + "Data_" + timestamp + ".txt";
        dataofs.open(datafn.c_str(), ios::app); 
        writePsecData(dataofs, boardsReadyForRead);
    }
    //eth.setBurstMode(false);

    return 0;
}


/*ID 27: Turn off triggers and data transfer off */
void ACC::endRun(unsigned int boardMask)
{
    setHardwareTrigSrc(0, boardMask);
    eth.setBurstMode(false);
    enableTransfer(0); 
}



/*------------------------------------------------------------------------------------*/
/*---------------------------Active functions for informations------------------------*/

/*ID 19: Pedestal setting procedure.*/
bool ACC::setPedestals(unsigned int boardmask, unsigned int chipmask, unsigned int adc)
{
    for(int iChip = 0; iChip < 5; ++iChip)
    {
	if(chipmask & (0x01 << iChip))
	{
	    unsigned int command = 0x00A20000;
	    command = (command | (boardmask << 24) ) | (iChip << 12) | adc;
            eth.send(0x100, command);
	}
    }
    return true;
}

/*ID 24: Special function to check connected ACDCs for their firmware version*/ 
void ACC::versionCheck(bool debug)
{
    unsigned int command;
	
    auto AccBuffer = eth.recieve_many(0x1000, 32);
    if(AccBuffer.size()==32)
    {
        std::cout << "ACC has the firmware version: " << std::hex << AccBuffer.at(0) << std::dec;
        uint16_t year  = (AccBuffer[1] >> 16) & 0xffff;
        uint16_t month = (AccBuffer[1] >>  8) & 0xff;
        uint16_t day   = (AccBuffer[1] >>  0) & 0xff;
        std::cout << " from " << std::hex << month << "/" << std::hex << day << "/" << std::hex << year << std::endl;

	if(debug)
	{
	    auto eAccBuffer = eth.recieve_many(0x1100, 64);

	    printf("  PLL lock status:\n    System PLL: %d\n    Serial PLL: %d\n    DPA PLL 1:  %d\n    DPA PLL 2:  %d\n", (AccBuffer[2] & 0x1)?1:0, (AccBuffer[2] & 0x2)?1:0, (AccBuffer[2] & 0x4)?1:0, (AccBuffer[2] & 0x8)?1:0);
	    printf("  %-30s %10s %10s %10s %10s %10s %10s %10s %10s\n", "", "ACDC0", "ACDC1", "ACDC2", "ACDC3", "ACDC4", "ACDC5", "ACDC6", "ACDC7");
	    printf("  %-30s %10d %10d %10d %10d %10d %10d %10d %10d\n", "40 MPBS link rx clk fail", (AccBuffer[16] & 0x1)?1:0, (AccBuffer[16] & 0x2)?1:0, (AccBuffer[16] & 0x4)?1:0, (AccBuffer[16] & 0x8)?1:0, (AccBuffer[16] & 0x10)?1:0, (AccBuffer[16] & 0x20)?1:0, (AccBuffer[16] & 0x40)?1:0, (AccBuffer[16] & 0x80)?1:0);
	    printf("  %-30s %10d %10d %10d %10d %10d %10d %10d %10d\n", "40 MPBS link align err", (AccBuffer[17] & 0x1)?1:0, (AccBuffer[17] & 0x2)?1:0, (AccBuffer[17] & 0x4)?1:0, (AccBuffer[17] & 0x8)?1:0, (AccBuffer[17] & 0x10)?1:0, (AccBuffer[17] & 0x20)?1:0, (AccBuffer[17] & 0x40)?1:0, (AccBuffer[17] & 0x80)?1:0);
	    printf("  %-30s %10d %10d %10d %10d %10d %10d %10d %10d\n", "40 MPBS link decode err", (AccBuffer[18] & 0x1)?1:0, (AccBuffer[18] & 0x2)?1:0, (AccBuffer[18] & 0x4)?1:0, (AccBuffer[18] & 0x8)?1:0, (AccBuffer[18] & 0x10)?1:0, (AccBuffer[18] & 0x20)?1:0, (AccBuffer[18] & 0x40)?1:0, (AccBuffer[18] & 0x80)?1:0);
	    printf("  %-30s %10d %10d %10d %10d %10d %10d %10d %10d\n", "40 MPBS link disparity err", (AccBuffer[19] & 0x1)?1:0, (AccBuffer[19] & 0x2)?1:0, (AccBuffer[19] & 0x4)?1:0, (AccBuffer[19] & 0x8)?1:0, (AccBuffer[19] & 0x10)?1:0, (AccBuffer[19] & 0x20)?1:0, (AccBuffer[19] & 0x40)?1:0, (AccBuffer[19] & 0x80)?1:0);
	    printf("  %-30s %10lu %10lu %10lu %10lu %10lu %10lu %10lu %10lu\n", "40 MPBS link Rx FIFO Occ", eAccBuffer[56], eAccBuffer[57], eAccBuffer[58], eAccBuffer[59], eAccBuffer[60], eAccBuffer[61], eAccBuffer[62], eAccBuffer[63]);
	    printf("  %-30s %10lu %10lu %10lu %10lu %10lu %10lu %10lu %10lu\n", "250 MPBS Byte FIFO 0 Occ", eAccBuffer[0], eAccBuffer[2], eAccBuffer[4], eAccBuffer[6], eAccBuffer[8], eAccBuffer[10], eAccBuffer[12], eAccBuffer[14]);
	    printf("  %-30s %10lu %10lu %10lu %10lu %10lu %10lu %10lu %10lu\n", "250 MPBS Byte FIFO 1 Occ", eAccBuffer[1], eAccBuffer[3], eAccBuffer[5], eAccBuffer[7], eAccBuffer[9], eAccBuffer[11], eAccBuffer[13], eAccBuffer[15]);
	    printf("  %-30s %10lu %10lu %10lu %10lu %10lu %10lu %10lu %10lu\n", "250 MPBS PRBS Err 0", eAccBuffer[16], eAccBuffer[18], eAccBuffer[20], eAccBuffer[22], eAccBuffer[24], eAccBuffer[26], eAccBuffer[28], eAccBuffer[30]);
	    printf("  %-30s %10lu %10lu %10lu %10lu %10lu %10lu %10lu %10lu\n", "250 MPBS PRBS Err 1", eAccBuffer[17], eAccBuffer[19], eAccBuffer[21], eAccBuffer[23], eAccBuffer[25], eAccBuffer[27], eAccBuffer[29], eAccBuffer[31]);
	    printf("  %-30s %10lu %10lu %10lu %10lu %10lu %10lu %10lu %10lu\n", "250 MPBS Symbol Err 0", eAccBuffer[32], eAccBuffer[34], eAccBuffer[36], eAccBuffer[38], eAccBuffer[40], eAccBuffer[42], eAccBuffer[44], eAccBuffer[46]);
	    printf("  %-30s %10lu %10lu %10lu %10lu %10lu %10lu %10lu %10lu\n", "250 MPBS Symbol Err 1", eAccBuffer[33], eAccBuffer[35], eAccBuffer[37], eAccBuffer[39], eAccBuffer[41], eAccBuffer[43], eAccBuffer[45], eAccBuffer[47]);
	    printf("  %-30s %10lu %10lu %10lu %10lu %10lu %10lu %10lu %10lu\n", "250 MBPS FIFO Occ", eAccBuffer[48], eAccBuffer[49], eAccBuffer[50], eAccBuffer[51], eAccBuffer[52], eAccBuffer[53], eAccBuffer[54], eAccBuffer[55]);
	    printf("\n");
	    //for(auto& val : lastAccBuffer) printf("%016lx\n", val);
	    //for(unsigned int i = 0; i < lastAccBuffer.size(); ++i) printf("stuff: 11%02x: %10ld\n", i, lastAccBuffer[i]);
	}
    }
    else
    {
        std::cout << "ACC got the no info frame" << std::endl;
    }

    //for(int i = 0; i < 16; ++i) printf("byteFIFO occ %5d: %10d\n", i, eth.recieve(0x1100+i));

    //flush ACDC slow control FIFOs 
    eth.send(0x2, 0xff);
	
    //Request ACDC info frame 
    command = 0xFFD00000; 
    eth.send(0x100, command);

    usleep(500);

    //Loop over the ACC buffer words that show the ACDC buffer size
    //32 words represent a connected ACDC
    for(int i = 0; i < MAX_NUM_BOARDS; i++)
    {
        uint64_t bufLen = eth.recieve(0x1138+i);
        if(bufLen > 5)
        {
            std::vector<uint64_t> buf = eth.recieve_many(0x1200+i, bufLen, EthernetInterface::NO_ADDR_INC);
            std::cout << "Board " << i << " has the firmware version: " << std::hex << buf.at(2) << std::dec;
            std::cout << " from " << std::hex << ((buf.at(4) >> 8) & 0xff) << std::dec << "/" << std::hex << (buf.at(4) & 0xff) << std::dec << "/" << std::hex << buf.at(3) << std::dec << std::endl;

	    if(debug)
	    {
		printf("  Header/footer: %4lx %4lx %4lx %4lx (%s)\n", buf[0], buf[1], buf[30], buf[31], (buf[0] == 0x1234 && buf[1] == 0xbbbb && buf[30] == 0xbbbb && buf[31] == 0x4321)?"Correct":"Wrong");
		printf("  PLL lock status:\n    ACC PLL:    %d\n    Serial PLL: %d\n    JC PLL:     %d\n    WR PLL:     %d\n", (buf[6] & 0x4)?1:0, (buf[6] & 0x2)?1:0, (buf[6] & 0x8)?1:0, (buf[6] & 0x1)?1:0);
		printf("  Backpressure:           %8d\n", (buf[5] & 0x2)?1:0);
		printf("  40 MBPS parity error:   %8d\n", (buf[5] & 0x1)?1:0);
		printf("  Event count:            %8lu\n", (buf[15] << 16) | buf[16]);
		printf("  ID Frame count:         %8lu\n", (buf[17] << 16) | buf[18]);
		printf("  Trigger count all:      %8lu\n", buf[19]);
		printf("  Trigger count accepted: %8lu\n", buf[20]);
		printf("  PSEC0 FIFO Occ:         %8lu\n", buf[21]);
		printf("  PSEC1 FIFO Occ:         %8lu\n", buf[22]);
		printf("  PSEC2 FIFO Occ:         %8lu\n", buf[23]);
		printf("  PSEC3 FIFO Occ:         %8lu\n", buf[24]);
		printf("  PSEC4 FIFO Occ:         %8lu\n", buf[25]);
		printf("  Wr time FIFO Occ:       %8lu\n", buf[26]);
		printf("  Sys time FIFO Occ:      %8lu\n", buf[27]);
		printf("\n");
	    }
            //printf("bufLen: %ld\n", bufLen);
            //for(unsigned int j = 0; j < buf.size(); ++j) printf("%3i: %016lx\n", j, buf[j]);
        }
        else
        {
            std::cout << "Board " << i << " is not connected" << std::endl;
        }
    }
}


/*------------------------------------------------------------------------------------*/
/*-------------------------------------Help functions---------------------------------*/

/*ID 13: Fires the software trigger*/
void ACC::softwareTrigger()
{
	//Software trigger
        eth.send(0x0010, 0xff);
}

/*ID 16: Used to dis/enable transfer data from the PSEC chips to the buffers*/
void ACC::enableTransfer(int onoff)
{
    unsigned int command;
    command = 0xFFF60000;
    eth.send(0x100, command + onoff);
}

/*ID 10: Clear all ACDC class instances*/
void ACC::clearAcdcs()
{
	for(int i = 0; i < (int)acdcs.size(); i++)
	{
		delete acdcs[i];
	}
	acdcs.clear();
}

/*ID 23: Wakes up the USB by requesting an ACC info frame*/
void ACC::usbWakeup()
{
//	unsigned int command = 0x00200000;
//	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
}

/*ID 18: Tells ACDCs to clear their ram.*/ 
void ACC::dumpData(unsigned int boardMask)
{
	//send and read.
        eth.send(0x0001, boardMask);
}


/*ID 27: Resets the ACDCs*/
void ACC::resetACDC()
{
    unsigned int command = 0xFFFF0000;
    eth.send(0x100, command);
    std::cout << "ACDCs were reset" << std::endl;
}

/*ID 28: Resets the ACCs*/
void ACC::resetACC()
{
    eth.send(0x0000, 0x1);
    std::cout << "ACCs was reset" << std::endl;
}


/*------------------------------------------------------------------------------------*/
/*-------------------------------------Help functions---------------------------------*/

/*ID 29: Write function for the error log*/
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
//    ss << std::put_time(std::localtime(&in_time_t), "%m-%d-%Y %X");
//    os_err << "------------------------------------------------------------" << endl;
//    os_err << ss.str() << endl;
//    os_err << errorMsg << endl;
//    os_err << "------------------------------------------------------------" << endl;
//    os_err.close();
}

/*ID 30: Write function for the raw data format*/
void ACC::writeRawDataToFile(const vector<uint64_t>& buffer, string rawfn)
{
    ofstream d(rawfn.c_str(), ios::app | ios::binary); 
    d.write(reinterpret_cast<const char*>(buffer.data()), buffer.size()*sizeof(uint64_t));
//    for(const uint64_t& k : buffer)
//    {
//        d << hex <<  k << " ";
//    }
//    d << "\n";
    d.close();
    return;
}

/*ID 31: Write function for the parsed data format*/
void ACC::writePsecData(ofstream& d, const vector<int>& boardsReadyForRead)
{
	string delim = " ";
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
				d << dec << (unsigned short)map_data[bi][ch][enm] << delim;
			}
			if(enm<(int)map_meta[bi].size())
			{
				d << hex << map_meta[bi][enm] << delim;

			}else
			{
				d << 0 << delim;
			}
		}
		d << endl;
	}
	d.close();
}

/*ID 25: Scan possible high speed link clock phases and select the optimal phase setting*/ 
void ACC::scanLinkPhase(unsigned int boardMask, bool print)
{
    std::vector<std::vector<uint64_t>> errors;

    if(print)
    {
        printf("Phase  ");
        for(int iChan = 0; iChan < 8; iChan += 2) printf("%25s %2d %21s", "ACDC:", iChan/2, " ");
        printf("\n      ");
        for(int iChan = 0; iChan < 8; ++iChan) printf("%12s %2d          ", "Channel:", iChan%2);
        printf("\n      ");
        for(int iChan = 0; iChan < 8; ++iChan) printf(" %10s %9s    ", "Encode err", "PRBS err");
        printf("\n");
    }
    
    for(int iOffset = 0; iOffset < 24; ++iOffset)
    {
        // advance phase one step (there are 24 total steps in one cock cycle)
        eth.send(0x0054, 0x0000);
        for(int iChan = 0; iChan < 8; ++iChan)
        {
            eth.send(0x0055, 0x0000 + iChan);
            eth.send(0x0056, 0x0000);
        }

        // transmit idle pattern to make sure link is aligned 
        enableTransfer(0);

        usleep(1000);

        //transmit PRBS pattern 
        enableTransfer(1);

        usleep(100);

        //reset error counters 
        eth.send(0x0053, 0x0000);

        usleep(1000);

        std::vector<uint64_t> decode_errors = eth.recieve_many(0x1120, 8);
        if(print)
        {
            std::vector<uint64_t> prbs_errors = eth.recieve_many(0x1110, 8);

            printf("%5d  ", iOffset);
            for(int iChan = 0; iChan < 8; ++iChan) printf("%10d %9d     ", uint32_t(decode_errors[iChan]), uint32_t(prbs_errors[iChan]));
            printf("\n");
        }

        errors.push_back(decode_errors);
    }

    // set transmitter back to idle mode
    enableTransfer(0);

    if(boardMask)
    {
        if(print) printf("Set:   "); 
        for(int iChan = 0; iChan < 8; ++iChan)
        {
            // set phase for chanels in boardMask
            if(boardMask & (1 << iChan))
            {
                int stop = 0;
                int length = 0;
                int length_best = 0;
                for(int i = 0; i < int(2*errors.size()); ++i)
                {
                    int imod = i % errors.size();
                    if(errors[imod][iChan] == 0)
                    {
                        ++length;
                    }
                    else
                    {
                        if(length >= length_best)
                        {
                            stop = imod;
                            length_best = length;
                        }
                        length = 0;
//                if(i > int(errors.size())) break;
                    }
                }
                int phaseSetting = (stop - length_best/2)%errors.size();
                if(print) printf("%15d          ", phaseSetting);
                eth.send(0x0054, 0x0000);
                eth.send(0x0055,  iChan);
                for(int i = 0; i < phaseSetting; ++i)
                {
                    eth.send(0x0056, 0x0000);	    
                }
            }
            else
            {
                if(print) printf("%25s", " ");
            }
        }
        if(print) printf("\n"); 

	// ensure at least 1 ms for links to realign (ensures at least 25 alignment markers)
	usleep(1000);

        //reset error counters 
        eth.send(0x0053, 0x0000);
    }

}

void ACC::sendJCPLLSPIWord(unsigned int word, unsigned int boardMask, bool verbose)
{
    unsigned int clearRequest = 0x00F10000 | (boardMask << 24);
    unsigned int lower16 = 0x00F30000 | (boardMask << 24) | (0xFFFF & word);
    unsigned int upper16 = 0x00F40000 | (boardMask << 24) | (0xFFFF & (word >> 16));
    unsigned int setPLL = 0x00F50000 | (boardMask << 24);
    
    eth.send(0x100, clearRequest);
    eth.send(0x100, lower16);
    eth.send(0x100, upper16);
    eth.send(0x100, setPLL);
    eth.send(0x100, clearRequest);

    if(verbose)
    {
	printf("send 0x%08x\n", lower16);
	printf("send 0x%08x\n", upper16);
	printf("send 0x%08x\n", setPLL);
    }
}

/*ID 26: Configure the jcPLL settings */
void ACC::configJCPLL(unsigned int boardMask)
{
    // program registers 0 and 1 with approperiate settings for 40 MHz output 
    //sendJCPLLSPIWord(0x55500060, boardMask); // 25 MHz input
    sendJCPLLSPIWord(0x5557C060, boardMask); // 125 MHz input
    usleep(2000);    
    //sendJCPLLSPIWord(0x83810001, boardMask); // 25 MHz input
    sendJCPLLSPIWord(0xFF810081, boardMask); // 125 MHz input
    usleep(2000);

    // cycle "power down" to force VCO calibration 
    sendJCPLLSPIWord(0x00001802, boardMask);
    usleep(2000);
    sendJCPLLSPIWord(0x00001002, boardMask);
    usleep(2000);
    sendJCPLLSPIWord(0x00001802, boardMask);
    usleep(2000);

    // toggle sync bit to synchronize output clocks
    sendJCPLLSPIWord(0x0001802, boardMask);
    usleep(2000);
    sendJCPLLSPIWord(0x0000802, boardMask);
    usleep(2000);
    sendJCPLLSPIWord(0x0001802, boardMask);
    usleep(2000);

    // read register
//    sendJCPLLSPIWord(0x0000000e);
//    sendJCPLLSPIWord(0x00000000);

    // write register contents to EEPROM
    //sendJCPLLSPIWord(0x0000000f);

}
