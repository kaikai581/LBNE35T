//==============================================================================
//
// Title:		Device.h
// Description:	This module contains the software for communicating with the
//				LBNE Digitizer.  It will support two connection types:
//				1)	The FTDI FT2232H_mini_module is a dual USB device with a
//					serial port for the control path and FIFO for the data path.
//				2)	A gigabit Ethernet link is also available to provide control
//					and data paths over UDP.  Initially, this method will not be
//					used.
// Author:		Andrew Kreps
//
//==============================================================================

#ifndef __DEVICE_H__
#define __DEVICE_H__

//==============================================================================
// Constants
//==============================================================================

#define MAX_CTRL_DATA		256
#define MAX_DEVICES			16
#define MAX_LENGTH_NAME		16
#define MAX_LENGTH_DESC		64

//==============================================================================
// Enumerated Constants
//==============================================================================

enum retryConstants {
	retryOff	= 0,
	retryOn		= 1
};

enum commConstants {
	commNone	= 0,
	commTCP		= 1,
	commUDP		= 2,
	commUSB		= 3
};

enum commandConstants {
	cmdNone			= 0,
	// Basic Commands
	cmdRead			= 1,
	cmdReadMask		= 2,
	cmdWrite		= 3,
	cmdWriteMask	= 4,
	// Array Commands
	cmdArrayRead	= 5,
	cmdArrayWrite	= 6,
	// Fifo Commands
	cmdFifoRead		= 7,
	cmdFifoWrite	= 8,
	// NV Write Commands
	cmdNVWrite		= 9,
	cmdNVArrayWrite	= 10,
	cmdNVEraseSector= 11,	// 4KB  - 16 pages
	cmdNVEraseBlock	= 12,	// 64KB - 256 pages
	cmdNVEraseChip	= 13,	// 16MB - 65536 pages
	numCommands
};

enum statusConstants {
	statusNoError			= 0,
	statusSendError			= 1,
	statusReceiveError		= 2,
	statusTimeoutError		= 3,
	statusAddressError		= 4,
	statusAlignError		= 5,
	statusCommandError		= 6,
	statusSizeError			= 7,
	statusReadError			= 8,		// Returned if read-only address is written
	statusWriteError		= 9,		// Returned if read-only address is written
	statusFlashReadError	= 10,
	statusFlashWriteError	= 11,
	statusFlashEraseError	= 12,
};

//==============================================================================
// Types
//==============================================================================

typedef struct _Device {
	char	name[MAX_LENGTH_NAME];
	int		ftDeviceA;	// Data Port
	int		ftDeviceB;	// Control Port
} Device;

typedef struct _Ctrl_Header {
	uint length;
	uint address;
	uint command;
	uint size;
	uint status;
} Ctrl_Header;

typedef struct _Ctrl_Packet {
	Ctrl_Header	header;
	uint		data[MAX_CTRL_DATA];
} Ctrl_Packet;

//==============================================================================
// Function Prototypes
//==============================================================================

// Initialization Functions
void DeviceInit (void);

// Connection Functions
int DeviceDiscover	(uint commType, uint* numDevices);
int DeviceConnect	(uint commType, int deviceNum, char* hostIP, char* deviceIP);
int DeviceDisconnect(uint commType, int deviceNum);

// Data Functions
int  DevicePurgeComm	(int deviceNum);
int  DevicePurgeData	(int deviceNum);
int  DeviceQueueStatus	(uint* numBytes, int deviceNum);
int  DeviceReceive		(Event_Packet* data, uint* dataReceived, int deviceNum);
int  DeviceTimeout		(uint timeout, int deviceNum);
uint DeviceLostData		(void);

// Command Functions
int DeviceRead		(uint address, uint* value, int deviceNum);
int DeviceReadMask	(uint address, uint mask, uint* value, int deviceNum);
int DeviceWrite		(uint address, uint value, int deviceNum);
int DeviceWriteMask	(uint address, uint mask, uint value, int deviceNum);
int DeviceSet		(uint address, uint mask, int deviceNum);	// Special case == DeviceWriteMask(address, mask, 0xFFFFFFFF);
int DeviceClear		(uint address, uint mask, int deviceNum);	// Special case == DeviceWriteMask(address, mask, 0x00000000);
int DeviceArrayRead	(uint address, uint size, uint* data, int deviceNum);
int DeviceArrayWrite(uint address, uint size, uint* data, int deviceNum);
int DeviceFifoRead	(uint address, uint size, uint* data, int deviceNum);
int DeviceFifoWrite		(uint address, uint size, uint* data, int deviceNum);
int DeviceNVWrite (uint address, uint value, int deviceNum);
int DeviceNVArrayWrite (uint address, uint size, uint* data, int deviceNum);
int DeviceNVEraseSector (uint address, int deviceNum);
int DeviceNVEraseBlock (uint address, int deviceNum);
int DeviceNVEraseChip (uint address, int deviceNum);

// Support Functions
int SendReceive			(Ctrl_Packet* tx, Ctrl_Packet* rx, uint retry, int deviceNum);
int SendReceiveUSB		(Ctrl_Packet* tx, Ctrl_Packet* rx, uint retry, int deviceNum);
int SendUSB				(Ctrl_Packet* tx, int deviceNum);
int ReceiveUSB			(Ctrl_Packet* rx, int deviceNum);
void RxErrorPacket 		(uint status, Ctrl_Packet* rx);

int ScanForEventStart	(void); 

// Script Functions
double DeviceSetupTime	(void);
int DeviceSetup			(int deviceNum);
int DeviceStart			(int deviceNum);
int DeviceStopReset		(int deviceNum);

#ifdef ANL
	// Log Functions
	void LogComment (const char format[], ...);
	void LogData	(void* data, uint dataBytes);
	void LogStart	(void);
	void LogStop	(void);
#endif

#endif
