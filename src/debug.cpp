#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <bitset>
#include <unistd.h>

using namespace std;

//this macro is a debugging function
//for sending and listening to data
//transmitted on the USB line. It circumvents
//the need for an ACC object and just isolates the usb.

//Arguments are either 0x1e0C0000 (command type 32 bit words)
//or a read command, specified as "r". One can string them in 
//any sequence you want. 


//printing utility
void printReadBuffer(vector<unsigned short> b)
{
	 
	int counter = 0;
	//loop through each vector element
	cout << "Size of vector is " << b.size() << endl;
	for(unsigned short val: b)
	{
		cout << val << ", "; //decimal
		stringstream ss;
		ss << std::hex << val;
		string hexstr(ss.str());
		cout << hexstr << ", "; //hex
		unsigned n;
		ss >> n;
		bitset<16> word(n);
		cout << word.to_string(); //binary
		cout << "," << counter << "th word number" << endl;
		counter++;
	}
}


int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
// NEEDS TO BE REWRITTEN FOR ETHERNET INTERFACE	
//
//	//number of 16 bit words to allocate memory for usb read
//	int usbReadBufferMax = 20000;
//
//	for(int c = 1; c < argc; c++)
//	{
//		usleep(100000); //100 ms delay
//		//read and move on
//		if(string(argv[c]) == "r")
//		{
//			vector<unsigned short> buff = usb->safeReadData(usbReadBufferMax);
//			printReadBuffer(buff);
//			continue;
//		}
//		else
//		{
//			//send data
//			stringstream ss;
//			unsigned int cmd;
//			ss << argv[c];
//			ss >> std::hex >> cmd;
//			cout << "Sending: " << std::hex << cmd << endl;
//			cout << std::dec;
//			usb->sendData(cmd);
//			continue;
//		}
//	}
//
//	delete usb;
	return 0;
}
