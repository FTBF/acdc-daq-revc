#include "ACDC.h"
#include <bitset>
#include <sstream>

using namespace std;

ACDC::ACDC()
{
	trigMask = 0xFFFFFF;
	convertMaskToChannels();
}



ACDC::~ACDC()
{
	cout << "Calling acdc detructor" << endl;
}

void ACDC::printMetadata(bool verbose)
{
	meta.standardPrint();
	if(verbose) meta.printAllMetadata();
}

void ACDC::setTriggerMask(unsigned int mask)
{
	trigMask = mask;
	convertMaskToChannels();
}

int ACDC::getBoardIndex()
{
	return boardIndex;
}

void ACDC::setBoardIndex(int bi)
{
	boardIndex = bi;
}

unsigned int ACDC::getTriggerMask()
{
	return trigMask;
}

vector<int> ACDC::getMaskedChannels()
{
	return maskedChannels;
}

//reads the value of unsigned int trigMask
//and converts it to a vector of ints 
//corresponding to channels that are masked, 
//i.e. inactive in the trigger logic. 
void ACDC::convertMaskToChannels()
{ 

	//clear the vector
	maskedChannels.clear();
	for(int i = 0; i < NUM_CH; i++)
	{
		if((trigMask & (1 << i)))
		{
			//channel numbering starts at 1
			maskedChannels.push_back(i + 1); 
		}
	}
}



bool ACDC::setLastBuffer(vector<unsigned short> b, int eventNumber)
{
	lastAcdcBuffer = b;
	bool goodBuffer = true;
	goodBuffer = meta.parseBuffer(b); //the BIG buffer parsing function that fills data/metadata maps
	meta.setBoardAndEvent((unsigned short)boardIndex, eventNumber);
	return goodBuffer;

}

//utility for debugging
void ACDC::writeRawBufferToFile()
{
    string fnnn = "raw-acdc-buffer.txt";
    cout << "Printing ACDC buffer to file : " << fnnn << endl;
    ofstream cb(fnnn);
    for(unsigned short k: lastAcdcBuffer)
    {
        printByte(cb, k);
        cb << endl;
    }
    cb.close();
}

void ACDC::printByte(ofstream& ofs, unsigned short val)
{
    ofs << val << ", "; //decimal
    stringstream ss;
    ss << std::hex << val;
    string hexstr(ss.str());
    ofs << hexstr << ", "; //hex
    unsigned n;
    ss >> n;
    bitset<16> b(n);
    ofs << b.to_string(); //binary
}


//looks at the last ACDC buffer and organizes
//all of the data into a data map. 
//retval: 
//2: other error
//1: corrupt buffer 
//0: all good
int ACDC::parseDataFromBuffer()
{
	//make sure an acdc buffer has been
	//filled. if not, there is nothing to be done.
	if(lastAcdcBuffer.size() == 0)
	{
		cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << endl;
		return 2;
	}

	if(peds.size() == 0 || conv.size() == 0)
	{
		cout << "Found no pedestal or LUT conversion data but was told to parse data." << endl;
		cout << "Please check the ACC class for an initialization of this calibration data" << endl;
		return 2;
	}

	//writeRawBufferToFile(); //debug

	//word that indicates the data is
	//about to start for each psec chip.
	unsigned short startword = 0xF005; 
	unsigned short endword = 0xBA11; //just for safety, uses the 256 sample rule. 
	int channelCount = 0; //iterates every 256 samples and when a startword is found 
	int sampleCount = 0; //for checking if we hit 256 samples. 
	double sampleValue = 0.0; //temporary holder for the sample in ADC-counts/mV
	bool dataFlag = false; //if we are currently on psec-data bytes. 
	vector<double> waveform; //the current channel's data as a vector of doubles. 
	//a for loop through the whole buffer and save states
	for(unsigned short byte: lastAcdcBuffer)
	{
		if(byte == startword && !dataFlag)
		{
			//re-initialize for a new PSEC chip
			dataFlag = true;
			sampleCount = 0;
			channelCount++;
			continue;
		}
		
		if(byte == endword)
		{
			dataFlag = false;
			
			//if the number
			//of datapoints in between 
			//the metadata packets is not
			//NUM_CH_PER_CHIP*NUM_SAMP (256*6)
			if(waveform.size() != 0)
			{
				//data map is not totally filled...
				cout << waveform.size() << endl;
				return 1;
			}
			continue;
		}

		if(dataFlag)
		{
			//here is where we assume every
			//channel to have NUM_SAMP samples. 
			if(sampleCount == NUM_SAMP)
			{
				sampleCount = 0;
				data[channelCount] = waveform;
				waveform.clear();
				channelCount++;
			}
			if(channelCount > NUM_CH)
			{
				//we are done here. 
				break; //could also be continue.
			}

			//---these lines fill a waveform vector
			//---that will be inserted into the data map
			sampleValue = (double)byte; //adc counts
			//apply a pedestal subtraction
			sampleValue = sampleValue - peds[channelCount][sampleCount]; //adc counts
			//apply a linearity corrected mV conversion
			sampleValue = sampleValue*conv[channelCount][sampleCount]; //mV
			//save in the vector. vector is saved in the data map when
			//the channel count is iterated. 
			waveform.push_back(sampleValue); 
			sampleCount++;
			continue;
		}
		
	}

	return 0;

}


//writes data from the presently stored event
// to file assuming file has header already
void ACDC::writeDataToFile(ofstream& d, ofstream& m)
{
	string delim = " ";

	//metadata part is simple and contained
	//in that class. 
	meta.writeMetadataToFile(m, delim);

	int evno = meta.getEventNumber();

	vector<double> waveform;//temporary storage of wave data
	int channel; //temporary storage of channel
	map<int, vector<double>>::iterator it;
	for(it = data.begin(); it != data.end(); ++it)
	{
		waveform = it->second;
		channel = it->first;
		//first three columns are event, board, channel
		d << evno << delim << boardIndex << delim << channel << delim;
		//loop the vector and print values
		for(double s: waveform)
		{
			d << s << delim;
		}
		d << endl;
	}
	return;
}











