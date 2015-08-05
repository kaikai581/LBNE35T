// Test for communications problems with Argonne board

// C++ includes
#include <iostream>
#include <string>
#include <stdint.h>
#include <cstdio>

// Argonne includes
#include "LBNEWare.h"
#include "Device.h"
#include "ftd2xx.h"

// Function Prototypes
void PreviousPacketInfo(void);

int main( int nArgs, char **args ) {

	std::cout << "Configuring Argonne Board:" << std::endl;

	uint data = 0;
	uint array[12];
	char msg[256];

	// Clear all FTDI devices
	DeviceInit();
	LBNE_Init();

	// Locate Argonne Board
	int errorCode(0);
	uint nDevices(0);
	errorCode = DeviceDiscover(commUSB,&nDevices);
	if ( nDevices != 1 ) {
		std::cout << "DeviceDiscover reports " << nDevices << " devices are connected. Expected 1, so quitting." << std::endl;
		return 1;
	}
	std::cout << "Discovered Argonne Board." << std::endl;

	// Connect to Argonne Board
	errorCode = DeviceConnect(commUSB,0, NULL, NULL);
	if ( errorCode != 0 ) {
		std::cout << "Failed to connect to board 0. ErrorCode " << errorCode << std::endl;
		return 1;
	}

	usleep(1000000); // 1 seconds
	std::cout << "Delay Over" << std::endl;

	uint delay = 1000; // 1 ms
	DeviceWrite(lbneReg.led_threshold[0], 0x1111);
	usleep(delay);
	DeviceWrite(lbneReg.led_threshold[1], 0x2222);
	usleep(delay);
	DeviceWrite(lbneReg.led_threshold[2], 0x3333);
	usleep(delay);
	DeviceWrite(lbneReg.led_threshold[3], 0x4444);
	usleep(delay);
	DeviceWrite(lbneReg.led_threshold[4], 0x5555);
	usleep(delay);
	DeviceWrite(lbneReg.led_threshold[5], 0x6666);
	usleep(delay);
	DeviceWrite(lbneReg.led_threshold[6], 0x7777);
	usleep(delay);
	DeviceWrite(lbneReg.led_threshold[7], 0x8888);
	usleep(delay);
	DeviceWrite(lbneReg.led_threshold[8], 0x9999);
	usleep(delay);
	DeviceWrite(lbneReg.led_threshold[9], 0xAAAA);
	usleep(delay);
	DeviceWrite(lbneReg.led_threshold[10], 0xBBBB);
	usleep(delay);
	DeviceWrite(lbneReg.led_threshold[11], 0xCCCC);
	usleep(delay);

	DeviceArrayRead(lbneReg.led_threshold[0], 12, array);
	for (int i = 0; i < 12; i++) {
	  sprintf(msg, "LED[%d] = %#08X", i, array[i]);
	  std::cout << msg << std::endl;
	}

	DeviceArrayRead(lbneReg.led_threshold[0], 12, array);
	for (int i = 0; i < 12; i++) {
	  sprintf(msg, "LED[%d] = %#08X", i, array[i]);
	  std::cout << msg << std::endl;
	}

	// This test will see if the ARM processor resets during run
	DeviceRead(lbneReg.armTest[0], &data);
	sprintf(msg, "ArmTest = %#08X at start", data);
	std::cout << msg << std::endl;

	DeviceWrite(lbneReg.armTest[0], 0xAAAAAAAA);
	DeviceRead(lbneReg.armTest[0], &data);
	sprintf(msg, "ArmTest = %#08X after write", data);
	std::cout << msg << std::endl;

	DeviceRead(lbneReg.armTest[0], &data);
	sprintf(msg, "ArmTest = %#08X (double check)", data);
	std::cout << msg << std::endl;

	// Perform a bunch of I/O expecting some to fail
	std::cout << "Compare error addresses from three different runs of DeviceSetup" << std::endl;
	std::cout << "DeviceSetup #1" << std::endl;
	DeviceSetup();
	PreviousPacketInfo();
	std::cout << "DeviceSetup #2" << std::endl;
	DeviceSetup();
	PreviousPacketInfo();
	std::cout << "DeviceSetup #3" << std::endl;
	DeviceSetup();
	PreviousPacketInfo();
	std::cout << std::endl;

	// Search for an I/O that times out
	// Query and print what the module reports as the previous packet
	// If it matches the timed out I/O, the ARM failed to send a response (or the PC did not receive it)
	// If it matches the packet before the timed out I/O, the ARM did not receive the timed out I/O
	uint done = 0;
	uint channel = 0;
	uint value = 0;
	uint error = 0;
	do {
		error = DeviceWrite(lbneReg.led_threshold[channel], value);
		if (error) {
			PreviousPacketInfo();
			done = 1;
		} else {
			value++;
			channel++;
			if (channel == MAX_CHANNELS) {
				channel = 0;
			}
		}
	} while (!done);
 
	DeviceWrite(lbneReg.led_threshold[0], 0x1111);
	DeviceWrite(lbneReg.led_threshold[1], 0x2222);
	DeviceWrite(lbneReg.led_threshold[2], 0x3333);
	DeviceWrite(lbneReg.led_threshold[3], 0x4444);
	DeviceWrite(lbneReg.led_threshold[4], 0x5555);
	DeviceWrite(lbneReg.led_threshold[5], 0x6666);
	DeviceWrite(lbneReg.led_threshold[6], 0x7777);
	DeviceWrite(lbneReg.led_threshold[7], 0x8888);
	DeviceWrite(lbneReg.led_threshold[8], 0x9999);
	DeviceWrite(lbneReg.led_threshold[9], 0xAAAA);
	DeviceWrite(lbneReg.led_threshold[10], 0xBBBB);
	DeviceWrite(lbneReg.led_threshold[11], 0xCCCC);

	DeviceArrayRead(lbneReg.led_threshold[0], 12, array);
	for (int i = 0; i < 12; i++) {
	  sprintf(msg, "LED[%d] = %#08X", i, array[i]);
	  std::cout << msg << std::endl;
	}

	DeviceArrayRead(lbneReg.led_threshold[0], 12, array);
	for (int i = 0; i < 12; i++) {
	  sprintf(msg, "LED[%d] = %#08X", i, array[i]);
	  std::cout << msg << std::endl;
	}

	// If the ARM processor resets, this value will NOT be 0xAAAAAAAA
	DeviceRead(lbneReg.armTest[0], &data);
	sprintf(msg, "ArmTest = %#08X at end", data);
	std::cout << msg << std::endl;

	DeviceRead(lbneReg.armTest[0], &data);
	sprintf(msg, "ArmTest = %#08X (double check)", data);
	std::cout << msg << std::endl;

	sprintf(msg, "If end value != start value, the ARM resets during the run!");
	std::cout << msg << std::endl;

	// Disconnect from Argonne Board
	errorCode = DeviceDisconnect(commUSB);

	// Done!
	std::cout << "Done with communication tests" << std::endl;

	return 0;
}

void PreviousPacketInfo(void)
{
	uint array[256];
        DeviceArrayRead(0x20, 12, array);
	std::cout << "ARM reports the previous command was:" << std::endl;
	char msg[256];
	sprintf(msg, "RX Address = %#08X", array[0]);
	std::cout << msg << std::endl;
	sprintf(msg, "RX Command = %#08X", array[1]);
	std::cout << msg << std::endl;
	sprintf(msg, "RX Size    = %#08X", array[2]);
	std::cout << msg << std::endl;
	sprintf(msg, "RX Status  = %#08X", array[3]);
	std::cout << msg << std::endl;
	sprintf(msg, "TX Address = %#08X", array[4]);
	std::cout << msg << std::endl;
	sprintf(msg, "TX Command = %#08X", array[5]);
	std::cout << msg << std::endl;
	sprintf(msg, "TX Size    = %#08X", array[6]);
	std::cout << msg << std::endl;
	sprintf(msg, "TX Status  = %#08X", array[7]);
	std::cout << msg << std::endl;
	sprintf(msg, "Packets Processed = %d", array[8]);
	std::cout << msg << std::endl;
}
