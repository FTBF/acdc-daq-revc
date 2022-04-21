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

using namespace std;

int main()
{
    EthernetInterface eth("192.168.133.107", "2007");
    
//    // reset ACC
//    if(!usb->sendData(0x00000000))
//    {
//    	printf("Send Error!!!\n");
//    }
//    
//    usleep(1000);
//    
//    // reset ACDC
//    if(!usb->sendData(0xffff0000))
//    {
//    	printf("Send Error!!!\n");
//    }
//    
//    usleep(1000);

    // reset ACC data input FIFOs
    eth.send(0x0001, 0xff);

    usleep(2000);

    // set pedestal values
//    eth.send(0x100, 0xffa20300);

//    usleep(100000);

    // disable all calib switches
    eth.send(0x100, 0xffc0ffff);

    // disable user calib input
    eth.send(0x100, 0xffc10000);

    // set trig mode 2 in ACC (HARDWAR TRIG)
    for(int i = 0; i < 8; ++i) eth.send(0x0030 + i, 1);

    // set ACDC high speed data outputs to IDLE 
    eth.send(0x100, 0xFFF60000);

    // set ACDC trigger mode to OFF
    eth.send(0x100, 0xFFB00000);

    // send manchester training pulse (backpressure and trig)
    eth.send(0x0060, 1);

    usleep(500);

    // Enable ACDC trig mode EXT
    eth.send(0x100, 0xFFB00001);

    // Enable ACDC data output mode 3 (DATA)
    eth.send(0x100, 0xFFF60003);

    usleep(1000);
    
    // reset ACC link error counters 
    eth.send(0x0053, 0x1);

    // send software trigger
    eth.send(0x0010, 0xff);

    eth.setBurstTarget();
    eth.setBurstMode(true);

    // lazy wait to ensure data is all received 
    usleep(1000);

    std::vector<uint64_t> bufferOccMany = eth.recieve_many(0x1130, 8);
    
    for(int i = 0; i < 8; ++i)
    {
        uint64_t bufferOcc = eth.recieve(0x1130+i);
        
        printf("ACDC %1d: %10ld %10ld\n", i, bufferOcc, bufferOccMany[i]);

        if(bufferOcc >= 7696)
        {
            eth.send(0x22, i);

            std::vector<uint64_t> data = eth.recieve_burst(1540);


            printf("Header start: %12lx\n", (data[1] >> 48) & 0xffff);
            printf("Event count:  %12ld\n", (data[1] >> 32) & 0xffff);
            printf("sys time: %16lu\n", ((data[1] << 32) & 0xffffffff00000000) | ((data[2] >> 32) & 0xffffffff));
            printf("wr time (s):  %12lu\n", (data[2]) & 0xffffffff);
            printf("wr time (ns): %12lu\n", (data[3] >> 32) & 0xffffffff);
            printf("Header end:   %12lx\n", data[4] & 0xffff);
            
        }
        
//            //printf("Channel : ");
//            //for(int i = 0; i < 5; ++i) printf("%6d%6d%6d%6d%6d%6d ", 0 + i*6, 1 + i*6, 2 + i*6, 3 + i*6, 4 + i*6, 5 + i*6);
//            //printf("\n");
//            //for(int i = 0; i < 256; ++i)
//            //{
//            //    printf("%7d : ", i);
//            //    for(int k = 0; k < 5; ++k)
//            //    {
//            //	printf("%6x%6x%6x%6x%6x%6x ",
//            //	       buffer[16 + i + 0*256 + k*(256*6)],
//            //	       buffer[16 + i + 1*256 + k*(256*6)],
//            //	       buffer[16 + i + 2*256 + k*(256*6)],
//            //	       buffer[16 + i + 3*256 + k*(256*6)],
//            //	       buffer[16 + i + 4*256 + k*(256*6)],
//            //	       buffer[16 + i + 5*256 + k*(256*6)]);
//            //    }
//            //    printf("\n");
//            //}

    }

    eth.setBurstMode(false);

    return 0;
}