#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <map>
#include <numeric>
#include "EthernetInterface.h"

#include "ACDC.h"

using namespace std;

int main(int argn, char * argc[])
{
    (void)argc;

    ACDC acdc;

    EthernetInterface eth("192.168.46.108", "2007");

    eth.setBurstTarget();
    eth.setBurstMode(true);
    
//    // reset ACC
//    if(!usb->sendData(0x00000000))
//    {
//    	printf("Send Error!!!\n");
//    }
//    
//    usleep(1000);
//    
    // reset ACDC
    eth.send(0x100, 0xffff0000);
    
    usleep(1000);

    // Enable ACDC data output IDLE
    //eth.send(0x100, 0xFFF60000);

    usleep(2000);

    // reset ACC data input FIFOs
    eth.send(0x0001, 0xff);

    usleep(2000);

    // set pedestal values
//    eth.send(0x100, 0xffa20300);

//    usleep(100000);

    // disable all calib switches
    eth.send(0x100, 0xffc0ffff);

    // disable user calib input
    eth.send(0x100, 0xffc10001);

    // set self trigger thresholds
    const unsigned int thresh = 0x750;
    for(int i = 0; i < 5; ++i)
    {	
	eth.send(0x100, 0xffa60000 | (i << 12) | thresh);
	eth.send(0x100, 0xffa70000 | (i << 12) | thresh);
	eth.send(0x100, 0xffa80000 | (i << 12) | thresh);
	eth.send(0x100, 0xffa90000 | (i << 12) | thresh);
	eth.send(0x100, 0xffaa0000 | (i << 12) | thresh);
	eth.send(0x100, 0xffab0000 | (i << 12) | thresh);
    }

    //set self trigger mask
    eth.send(0x100, 0xffb1003f);
    eth.send(0x100, 0xffb1103f);
    eth.send(0x100, 0xffb1203f);
    eth.send(0x100, 0xffb1303f);
    eth.send(0x100, 0xffb1403f);

    // set trig mode 2 in ACC (HARDWARE TRIG)
    for(int i = 0; i < 8; ++i) eth.send(0x0030 + i, 0);

    // set ACDC high speed data outputs to IDLE 
    //eth.send(0x100, 0xFFF60000);

    // set ACDC trigger mode to OFF
    eth.send(0x100, 0xFFB00000);

    usleep(10000);

    // send manchester training pulse (backpressure and trig)
    eth.send(0x0060, 1);

    usleep(500);

    // Enable ACDC trig mode EXT
    eth.send(0x100, 0xFFB00002);

    // Enable ACDC data output mode 3 (DATA)
    eth.send(0x100, 0xFFF60003);

    // set trig mode 2 in ACC (HARDWAR TRIG)
    for(int i = 0; i < 8; ++i) eth.send(0x0030 + i, 0);

    usleep(1000);
    
    // reset ACC link error counters 
    eth.send(0x0053, 0x1);

    // send software trigger
    eth.send(0x0010, 0xff);

    // lazy wait to ensure data is all received 
    usleep(50000);

    for(int i = 0; i < 8; ++i)
    {
        uint64_t bufferOcc = eth.recieve(0x1130+i);
        
	printf("ACDC %1d: %10ld\n", i, bufferOcc);
        while(bufferOcc >= 7696)
        {
	  printf("ACDC %1d: %10ld\n", i, bufferOcc);

            eth.send(0x22, i);

            std::vector<uint64_t> data = eth.recieve_burst(1541);


            printf("Header start: %12lx\n", (data[1] >> 48) & 0xffff);
            printf("Event count:  %12ld\n", (data[1] >> 32) & 0xffff);
            printf("sys time: %16lu\n", data[2]);
            printf("wr time (s):  %12lu\n", (data[3] >> 32) & 0xffffffff);
            printf("wr time (ns): %12lu\n", (data[3]) & 0xffffffff);
            printf("Header end:   %12lx\n", data[4] & 0xffff);
	    
	    acdc.parseDataFromBuffer(data);
	    auto parsedData = acdc.returnData();

	    if(argn > 1)
	    {
		printf("Channel : ");
		for(int i = 0; i < 5; ++i) printf("%6d%6d%6d%6d%6d%6d ", 0 + i*6, 1 + i*6, 2 + i*6, 3 + i*6, 4 + i*6, 5 + i*6);
		printf("\n");
		for(int i = 0; i < 256; ++i)
		{
		    printf("%7d : ", i);
		    for(int k = 0; k < 5; ++k)
		    {
			printf("%6x%6x%6x%6x%6x%6x ",
			       parsedData[k*6 + 0][i],
			       parsedData[k*6 + 1][i],
			       parsedData[k*6 + 2][i],
			       parsedData[k*6 + 3][i],
			       parsedData[k*6 + 4][i],
			       parsedData[k*6 + 5][i]);
		    }
		    printf("\n");
		}
	    }


	    bufferOcc = eth.recieve(0x1130+i);
        }
        

    }

    eth.setBurstMode(false);

    // set ACDC high speed data outputs to IDLE 
    //    eth.send(0x100, 0xFFF60000);

    // set ACDC trigger mode to OFF
    //eth.send(0x100, 0xFFB00000);


    return 0;
}
