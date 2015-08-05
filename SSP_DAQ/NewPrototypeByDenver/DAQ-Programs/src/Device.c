//==============================================================================
//
// Title:		Device.c
// Description:	This module contains the software for communicating with the
//				LBNE Digitizer.  It will support two connection types:
//				1)	The FTDI FT2232H_mini_module is a dual USB device with a
//					serial port for the control path and FIFO for the data path.
//				2)	A gigabit Ethernet link is also available to provide control
//					and data paths over UDP.  Initially, this method will not be
//					used.
// Author:		Andrew Kreps (TCP functionally added by Michael Oberling)
//
//==============================================================================

//==============================================================================
// Include files
//==============================================================================

#include "LBNEWare.h"
#include "Device.h"
#include "ftd2xx.h"
#include <unistd.h>

#ifndef ANL
	// C++ includes for IU debugging
	#include <iostream>
	#include <iomanip>
	#include <string>
	#include <stdint.h>
        #include <cstdio>
        #include <cstring>

		
#endif

//==============================================================================
// Variables
//==============================================================================

// Shared TCP/UDP Connection Variables
uint	ControlPort0	= 55001;
uint	ControlPort1	= 55002;
uint	DataPort		= 55010;
char	DeviceIP[20]	= "192.168.1.101";
char	HostIP[20]		= "192.168.1.100";

// TCP Connection Variables
unsigned int	tcpControlHandle;
//unsigned int	tcpControlHandle2;
int				tcpControlCallbackData	= 0;
int 			tcpControlCallbackFunction (unsigned handle, int xType, int errCode, void *callbackData) {return 0;};	// Place holder function
unsigned int	tcpControlTimeout 		= 5000;
unsigned int	tcpDataHandle;
int				tcpDataCallbackData		= 0;
int 			tcpDataCallbackFunction (unsigned handle, int xType, int errCode, void *callbackData) {return 0;};   	// Place holder function
unsigned int	tcpDataTimeout 			= 5000;
uint			TCPdata[20000];

// USB_D2XX Connection Variables 
FT_HANDLE	ftControlDev[MAX_DEVICES];
FT_HANDLE	ftDataDev[MAX_DEVICES];
char		ftDescription[MAX_LENGTH_DESC];
char		ftSerial[MAX_LENGTH_NAME];
DWORD		ftComPort;
DWORD		ftFlags;
DWORD		ftID;
DWORD		ftLocation;
DWORD		ftNumDevs;
DWORD		ftType;
DWORD		ftReadTimeout	= 1000;	// in ms
DWORD		ftWriteTimeout	= 1000;	// in ms
UCHAR		ftLatency		= 2;	// in ms

// UDP Connection Variables
uint	udpControlChannel	= 0;
// Not supported	uint	udpDataChannel		= 0;

// Control Path Variables
Device		device[MAX_DEVICES];
uint		savedCommType;

// Data Path Variables
DWORD	dataLost		= 0;
int		dataSynced		= 0;
// Data Path Settings
DWORD	dataBufferSize	= 0x10000;	// 65536 bytes  - USB Buffer Size on PC
UCHAR	dataLatency		= 2;		// in ms
UCHAR	dataLatencyOld	= 0;
DWORD	dataTimeout		= 1000;		// in ms

#ifdef ANL
	char	logging = 0;
	char	logEntry[MAX_PATHNAME_LEN];
	int		logEvent = 0;
	FILE*	logFile;
	char	logFilename[MAX_PATHNAME_LEN];
	time_t	logMachineTime;
	struct tm* logTime;
#else
	uint postSendDelay		= 100;	// (us) - This delay is after the send portion of all control transactions
									//			(This delay is during transactions)
	uint postReceiveDelay	= 2000;	// (us) - This delay is after the receive portion of all control transactions
									//			(This delay is between transactions)
#endif
	
uint	startTime	= 0;
uint	endTime		= 0;
double	elapsedTime	= 0;
char	errString[MAX_PATHNAME_LEN];

//==============================================================================
// Initialization Functions
//==============================================================================

void DeviceInit (void)
{
	int i = 0;
	
	// Clear Device List
	for (i = 0; i < MAX_DEVICES; i++) {
		strcpy(device[i].name, "");
		device[i].ftDeviceA = -1;
		device[i].ftDeviceB = -1;
	}
}

//==============================================================================
// Connection Functions
//==============================================================================

int DeviceDiscover (uint commType, uint* numDevices)
{
	int error = 0;
	int currDevice = 0;
	uint numFound = 0;
	int length = 0;
	char serial[MAX_LENGTH_NAME];
	DWORD i = 0;
	DWORD j = 0;
	FT_STATUS ftStatus;
	
	switch (commType) {
		case commNone:
			// This option only for debugging a "connected" device
			break;
		case commTCP:
		case commUDP:
			// No  discovery for these interfaces.  Assuming static IP.
			break;
			
		case commUSB:
			// This code uses FTDI's D2XX driver to determine the available devices
			ftStatus = FT_CreateDeviceInfoList(&ftNumDevs);
			std::cout << "DEBUGINFO: FT_CreateDeviceInfoList: " << (ftStatus == FT_OK) << " (" << ftNumDevs << ")" << std::endl;
			if (ftStatus != FT_OK) {
				error = errorCommDiscover;
				break;
			}
			
			// Clear Device List
			for (i = 0; i < MAX_DEVICES; i++) {
				strcpy(device[i].name, "");
				device[i].ftDeviceA = -1;
				device[i].ftDeviceB = -1;
			}
			
			// 
			for (i = 0; i < ftNumDevs; i++) {
				ftStatus = FT_GetDeviceInfoDetail(i, &ftFlags, &ftType, &ftID, &ftLocation, ftSerial, ftDescription, &ftControlDev[i]);
				//std::cout << "FT_GetDeviceInfoDetail (device " << i << " of " << ftNumDevs << "): " << (ftStatus == FT_OK) << std::endl;
				//std::cout << "\tFlags " << ftFlags << std::endl;
				//std::cout << "\tType " << ftType << std::endl;
				//std::cout << "\tID " << ftID << std::endl;
				//std::cout << "\tLocation " << ftLocation << std::endl;
				//std::cout << "\tSerial " << ftSerial << std::endl;
				//std::cout << "\tDescription " << ftDescription << std::endl;
				//std::cout << "\tControlDev " << ftControlDev << std::endl;
				if (ftStatus != FT_OK) {
					error = errorCommDiscover;
				} else {
					// Search only for devices we can use (skip any others)
					// NOTE: There is a rare error in which the FTDI driver returns FT_OK but the device info is incorrect
					// In this case, the check on ftType and/or length of ftSerial will fail
					// The discover code will therefore fail to find any usable devices but will not return an error
					// The calling code should check numDevices and rerun DeviceDiscover if it equals zero
					if (ftType != FT_DEVICE_2232H) {
					  std::cout << "Failed ftType Check." << std::endl;
						continue;	// Skip to next device
					}
					
					// Add FTDI device to Device List using Serial number
					length = strlen(ftSerial);				// Find length of serial number (including 'A' or 'B')
					if (length == 0) {
					  std::cout << "Failed ftSerial Check." << std::endl;
						continue;	// Skip to next device
					}
					strncpy(serial, ftSerial, length - 1);	// Copy base serial number
					serial[length-1] = 0;					// Append NULL because strncpy() didn't!
					
					// Search existing devices for matching serial number
					currDevice = -1;
					for (j = 0; j < numFound; j++) {
						// NOTE: This code will not catch the case where multiple devices have the same serial number!
						if (strcmp(serial, device[j].name) == 0) {
							currDevice = j;
						}
					}
					
					// If no match, this is a new device
					if (currDevice == -1) {
						currDevice = numFound;
						strcpy(device[currDevice].name, serial);
						numFound++;
					}
					
					// Update device list with FTDI device number
					switch (ftSerial[length -1]) {
						case 'A':
							device[currDevice].ftDeviceA = i;
							continue;
						case 'B':
							device[currDevice].ftDeviceB = i;
							continue;
						default:
							continue;
					}
				}
			}
			break;
		default:
			error = errorCommType;
			break;
	}
	
	// Return number of devices found
	*numDevices = numFound;
	
	return error;
}

int DeviceConnect (uint commType, int deviceNum, char* hostIP, char* deviceIP)
{
	int error = 0;
	FT_STATUS ftStatus;

	// Save commType for use by SendReceive()
	savedCommType = commType;

	switch (commType) {
		case commNone:
			// This option only for debugging a "connected" device
			break;
		case commTCP:
			sprintf(HostIP,		hostIP);
			sprintf(DeviceIP,	deviceIP);
			
			#ifdef ANL_TCP
				// Open the slow control connection.
				error = ConnectToTCPServer(&tcpControlHandle, ControlPort0, DeviceIP, tcpControlCallbackFunction, &tcpControlCallbackData, tcpControlTimeout);
				if (error) {
					error = errorCommConnect;
					break;
				}
		
				// It's perfectly fine to open multiple slow control connections to the SSP.
				// However, due to a limitation in the current firmware only two connection are supported and they must connected to ports 55001, and 55002.
				// In our code the monitor process will use this second connection, and the GUI controls, and test procedures will use the first.
				//
				// Commented out for distribution
				//error = ConnectToTCPServer(&tcpControlHandle2, ControlPort1, DeviceIP, tcpControlCallbackFunction, &tcpControlCallbackData, tcpControlTimeout);
				//if (error) {
				//	error = errorCommConnect;
				//	break;
				//}
				
				// Open the event data connection.
				error = ConnectToTCPServer(&tcpDataHandle, DataPort, DeviceIP, tcpDataCallbackFunction, &tcpDataCallbackData, tcpDataTimeout);
				if (error) {
					error = errorCommConnect;
					break;
				}
				else
				{
					// Set the event data interface to Ethernet
					DeviceWrite(lbneReg.eventDataInterfaceSelect, 	0x00000001);
				}    	
			#endif              
			break;
			
		case commUDP:
			sprintf(HostIP,		hostIP);
			sprintf(DeviceIP,	deviceIP);
			
			#ifdef ANL_UDP
				// Create UDP channels for Control and Data paths
				// Arguments to CreateUDPChannelConfig() in order:
				//		localPort
				//		localAddress
				//		exclusive - non-zero means this port is reserved for this application
				//		callbackFunction
				//		callbackData
				//		channel - returned channel ID number
			
				error = CreateUDPChannelConfig(ControlPort0, HostIP, 1, NULL, 0, &udpControlChannel);
			#endif   
			if (error) {
				error = errorCommConnect;
				break;
			}
			
			break;
		case commUSB:
			if (deviceNum < 0) {
				error = errorCommConnect;
				break;
			}
			
			// This code uses FTDI's D2XX driver for the control and data paths
			// Open the Control Path
			ftStatus = FT_Open(device[deviceNum].ftDeviceB, &ftControlDev[deviceNum]);
			if (ftStatus != FT_OK) {
			        std::cout << "Error on FT_Open" << std::endl;
				error = errorCommConnect;
				break;
			}
			
			// Setup UART parameters for control path
			if (FT_SetBaudRate(ftControlDev[deviceNum], FT_BAUD_921600) != FT_OK) {
			        std::cout << "Error on FT_BAUD_921600" << std::endl;
				error = errorCommConnect;
			}
			if (FT_SetDataCharacteristics(ftControlDev[deviceNum], FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE) != FT_OK) {
			        std::cout << "Error on FT_SetDataCharacteristics" << std::endl;
				error = errorCommConnect;
			}
			if (FT_SetTimeouts(ftControlDev[deviceNum], ftReadTimeout, ftWriteTimeout) != FT_OK) {
			        std::cout << "Error on FT_SetTimeouts" << std::endl;
				error = errorCommConnect;
			}
			if (FT_SetFlowControl(ftControlDev[deviceNum], FT_FLOW_NONE, 0, 0) != FT_OK) {
			        std::cout << "Error on FT_SetFlowControl" << std::endl;
				error = errorCommConnect;
			}   
			if (FT_SetLatencyTimer(ftControlDev[deviceNum], ftLatency) != FT_OK) {
			        std::cout << "Error on FT_SetLatencyTimer" << std::endl;
				error = errorCommConnect;
			}   
			if (error) {								 
				// Error during control path setup, close already open device and break
				FT_Close(ftControlDev);
				break;
			}
			
			// Open the Data Path
			ftStatus = FT_Open(device[deviceNum].ftDeviceA, &ftDataDev[deviceNum]);
			if (ftStatus != FT_OK) {
				error = errorDataConnect;
				// Since Control Path is already open, close it now
				FT_Close(ftControlDev);
			}
			
			// Setup parameters for data path
			if (FT_GetLatencyTimer(ftDataDev[deviceNum], &dataLatencyOld) != FT_OK) {
				error = errorDataConnect;
			}
			if (FT_SetLatencyTimer(ftDataDev[deviceNum], dataLatency) != FT_OK) {
				error = errorDataConnect;
			}
			if (FT_SetUSBParameters(ftDataDev[deviceNum], dataBufferSize, dataBufferSize) != FT_OK) {
				error = errorDataConnect;
			}
			if (FT_SetTimeouts(ftDataDev[deviceNum], dataTimeout, 0) != FT_OK) {
				error = errorDataConnect;
			}
			if (FT_SetFlowControl(ftDataDev[deviceNum], FT_FLOW_RTS_CTS, 0, 0) != FT_OK) {
				error = errorDataConnect;
			}
			
			if (error) {
				// Error during data path setup, close already open devices
				FT_Close(ftControlDev);
				FT_Close(ftDataDev);
			}				
			else
			{
				// Set the event data interface to USB
			        DeviceWrite(lbneReg.eventDataInterfaceSelect, 	0x00000000, deviceNum);
			}    	
			
			break;
		default:
			error = errorCommType;
			break;
	}
	
	if (error == 0) {
#ifdef ANL
		LogStart();
#endif
	}

	return error;
}

int DeviceDisconnect (uint commType, int deviceNum)
{
	int error = 0;
	FT_STATUS ftStatus;

	switch (commType) {
		case commNone:
			// This option only for debugging a "connected" device
			break;
		case commTCP:
			#ifdef ANL_TCP
				error |= DisconnectFromTCPServer(tcpControlHandle);
				//error |= DisconnectFromTCPServer(tcpControlHandle2);
				error |= DisconnectFromTCPServer(tcpDataHandle); 
			#endif

			if (error) {
				error = errorDataDisconnect;
			}
			break;
		case commUDP:
			#ifdef ANL_UDP
				error = DisposeUDPChannel(udpControlChannel);
			#endif
			if (error) {
				error = errorCommDisconnect;
				break;
			}

		break;
		case commUSB:
			// This code uses FTDI's D2XX driver for the control and data paths
			// It seems to be necessary to manually flush the data path RX buffer
			// to avoid crashing LBNEWare when Disconnect is pressed
			error = DeviceStopReset(deviceNum);	// Must stop device so purge can work!
			error = DevicePurgeData(deviceNum);
			error = DevicePurgeComm(deviceNum);
			
			ftStatus = FT_Close(ftControlDev[deviceNum]);
			if (ftStatus != FT_OK) {
				error = errorCommDisconnect;
			}
			ftStatus = FT_Close(ftDataDev[deviceNum]);
			if (ftStatus != FT_OK) {
				error = errorDataDisconnect;
			}
			break;
		default:
			error = errorCommType;
			break;
	}
	
#ifdef ANL
	LogStop();
#endif
	
	return error;
}			

//==============================================================================
// Data Functions
//==============================================================================

int DeviceTimeout (uint timeout, int deviceNum)
{
	int error = 0;
	FT_STATUS ftStatus;

	switch (savedCommType) {
		case commNone:
			// This option only for debugging a "connected" device
			break;
		case commTCP:
			tcpControlTimeout = timeout;
			tcpDataTimeout = timeout;
			break;
		case commUSB:
			ftStatus = FT_SetTimeouts(ftDataDev[deviceNum], timeout, 0);
			if (ftStatus != FT_OK) {
				error = errorDataConnect;
			}
			break;
		default:
			error = errorCommType;
			break;
	}

	return error;
}



int DevicePurgeComm (int deviceNum)
{
	int		error = 0;
	int		done = 0;
	char	bytes[256];
	DWORD	bytesQueued = 0;
	DWORD	bytesReturned = 0;
	FT_STATUS ftStatus;
	#ifdef ANL
		int	state = 0;
	#endif
		
	switch (savedCommType) {
		case commNone:
			// This option only for debugging a "connected" device
			break;
			
		case commTCP:
			#ifdef ANL_TCP
				error = DisconnectFromTCPServer(tcpControlHandle);
				if (error) {
					error = errorCommDisconnect;
					break;
				}

				error = ConnectToTCPServer(&tcpControlHandle, ControlPort0, DeviceIP, tcpControlCallbackFunction, &tcpControlCallbackData, tcpControlTimeout);
				if (error) {
					error = errorCommConnect;
					break;
				}
			#endif	
			break;
			
		case commUSB:	
			#ifdef ANL
				// Disable error popups so timeouts can occur in Debug Mode
				state = SetBreakOnLibraryErrors(0);
			#endif
	
				do {
					ftStatus = FT_GetQueueStatus(ftControlDev[deviceNum], &bytesQueued);
					if (ftStatus != FT_OK) {
						error = errorCommPurge;
					} else if (bytesQueued > 256) {
						ftStatus = FT_Read(ftControlDev[deviceNum], (LPVOID*)&bytes, 256, &bytesReturned);
						if (ftStatus != FT_OK) {
							error = errorCommPurge;
						}
					} else if (bytesQueued > 0) {
						ftStatus = FT_Read(ftControlDev[deviceNum], (LPVOID*)&bytes, bytesQueued, &bytesReturned);
						if (ftStatus != FT_OK) {
							error = errorCommPurge;
						}
					}
		
					if (error) {
						done = 1;
					} else if (bytesQueued == 0) {
						#if defined(ANL)
							Delay(0.01);	// 10ms
						#else
							usleep(10000);	// 10ms
						#endif
						ftStatus = FT_GetQueueStatus(ftControlDev[deviceNum], &bytesQueued);
						if (bytesQueued == 0) {
							done = 1;
						}
					}
				} while (!done);

			#ifdef ANL
				// Restore previous state of error popups in Debug Mode
				SetBreakOnLibraryErrors(state);
			#endif
			
			break;
		default:
			error = errorCommType;
			break;
	}
	
	return error;
}

int DevicePurgeData (int deviceNum)
{
	int		error = 0;
	int		done = 0;
	char	bytes[256];
	DWORD	bytesReturned = 0;
	DWORD	bytesQueued = 0;
	FT_STATUS ftStatus;
	#ifdef ANL_TCP
		int		dataReturned = 0;
	#endif
	#ifdef ANL
		int		state = 0;
	#endif
	
	switch (savedCommType) {
		case commNone:
			// This option only for debugging a "connected" device
			break;
			
		case commTCP: 	
			#ifdef ANL_TCP
				#ifdef ANL
					// Disable error popups so timeouts can occur in Debug Mode
					state = SetBreakOnLibraryErrors(0);
				#endif

				do
				{
					dataReturned = ClientTCPRead(tcpDataHandle, (uint*)(TCPdata), 1000, 100); 
				} while(dataReturned > 0);

				#ifdef ANL
					// restore last state
					SetBreakOnLibraryErrors(state);
				#endif
			#endif
			break;

		case commUSB:	
			#ifdef ANL
				// Disable error popups so timeouts can occur in Debug Mode
				state = SetBreakOnLibraryErrors(0);
			#endif
	
			do {
				ftStatus = FT_GetQueueStatus(ftDataDev[deviceNum], &bytesQueued);
				if (ftStatus != FT_OK) {
					error = errorDataPurge;
				} else if (bytesQueued > 256) {
					ftStatus = FT_Read(ftDataDev[deviceNum], (LPVOID*)&bytes, 256, &bytesReturned);
					if (ftStatus != FT_OK) {
						error = errorDataPurge;
					}
				} else if (bytesQueued > 0) {
					ftStatus = FT_Read(ftDataDev[deviceNum], (LPVOID*)&bytes, bytesQueued, &bytesReturned);
					if (ftStatus != FT_OK) {
						error = errorDataPurge;
					}
				}
	
				if (error) {
					done = 1;
				} else if (bytesQueued == 0) {
					#if defined(ANL)
						Delay(0.01);	// 10ms
					#else
						usleep(10000);	// 10ms
					#endif
					ftStatus = FT_GetQueueStatus(ftDataDev[deviceNum], &bytesQueued);
					if (bytesQueued == 0) {
						done = 1;
					}
				}
			} while (!done);

			#ifdef ANL
				// Restore previous state of error popups in Debug Mode
				SetBreakOnLibraryErrors(state);
			#endif
			
			break;
		default:
			error = errorCommType;
			break;
	}	
	return error;
}

int DeviceQueueStatus (uint* numBytes, int deviceNum)
{
	int error = 0;
	FT_STATUS ftStatus;
	
	switch (savedCommType) {
		case commNone:
			// This option only for debugging a "connected" device
			break;

		case commTCP: 
			#ifdef ANL_TCP
			// The LabWindows driver does not provide any means of checking how many bytes are in the system buffer.
			#endif
		case commUSB:
			ftStatus = FT_GetQueueStatus(ftDataDev[deviceNum], numBytes);
			//std::cout << " FT_GetQueuStatus Got " << *numBytes << " bytes." << std::endl;
			if (ftStatus != FT_OK) {
				// Error getting Queue Status
			        std::cout << "Error in FT_GetQueueStatus" << std::endl;
				error = errorDataQueue;
			}
			break;
		default:
			error = errorCommType;
			break;
	}	
	
	return error;
}

int DeviceReceive (Event_Packet* data, uint* dataReceived, int deviceNum)
{
	int				error = 0;
	DWORD			dataExpected	= 0;
	uint				dataReturned	= 0;
	Event_Packet*	event = data;
	LPVOID*			dataBuff = (LPVOID*)data;
	uint			value = 0;
	FT_STATUS		ftStatus;
	#ifdef ANL
		int	state = 0;
	#endif

#ifdef ANL
	LogComment("EVENT: %d \n", logEvent);

	// Disable error popups so timeouts can occur in Debug Mode
	state = SetBreakOnLibraryErrors(0);
#endif
	

	switch(savedCommType)
	{
		case commTCP:
			#ifdef ANL_TCP
				dataSynced		= 1;	// Initially assume that the data is aligned.  This will be tested, so it's fine if it is not/
										// When collecting multiple events only the first call will likely not be synced, and it is
										// faster to test for the synced condition than to always resynchronize.
				dataExpected	= sizeof(Event_Header);	// Again, for efficiency assume everything is lined up and ready to go until proven otherwise. 
				dataLost		= 0;
				*dataReceived	= 0;
					
				/* 
				 *	Bandwidth testing only
				 *
				 * while(1)
				 * {
				 * 	dataReturned = ClientTCPRead(tcpDataHandle, (uint*)(TCPdata), 20000, tcpDataTimeout); 
				 * }
				 */
			
				dataReturned = ClientTCPRead(tcpDataHandle, (uint*)(data), dataExpected, tcpDataTimeout); 
				if(dataReturned == kTCP_TimeOutErr) 
				{
					error = errorDataTimeout;
				} 
				else if (dataReturned < kTCP_NoError)	// Won't accept for anything "less than" no errors.
				{
					// Error during receive
					error = errorDataReceive;
				} 
				else if (dataReturned != dataExpected) 
				{
					error = errorDataTimeout;
					dataLost += dataReturned;
				}
				else if (data->header.header != 0xAAAAAAAA) 
				{
					dataSynced = 0;
					// Throw this data away and increment counter
					dataLost += dataReturned;
				}
				else
				{
					*dataReceived += dataReturned;
				}
			
				if (error) break;
				
				if(dataSynced ==0)
				{
					dataExpected = 1;
					dataSynced = -3;
					while (dataSynced != 1)	// if we are not aligned then continue the alignment process.
					{
						dataReturned = ClientTCPRead(tcpDataHandle, (uint*)(data), dataExpected, tcpDataTimeout); 
						if(dataReturned == kTCP_TimeOutErr) 
						{
							error = errorDataTimeout;
						} 
						else if (dataReturned < kTCP_NoError) // Won't accept for anything "less than" no errors.
						{
							// Error during receive
							error = errorDataReceive;
						} 
						else if (dataReturned != dataExpected) 
						{
							error = errorDataTimeout;
							dataLost += dataReturned;
						}
						else if((((uint*)(data))[0] & 0x000000FF) == 0xAA) 
						{
							dataSynced++;
							*dataReceived += dataReturned;
						}
						else 
						{
							dataSynced = -3;
							// Throw this data away and increment counter
							dataLost += dataReturned;
						}
					
						if (error) break;
					}
				
					if (error) break;
				
					data->header.header = 0xAAAAAAAA;
				
					// now that we're synced, read the rest of the header:
					dataExpected = sizeof(Event_Header) - sizeof(uint);
					dataReturned = ClientTCPRead(tcpDataHandle, &(((uint*)(data))[1]), dataExpected, tcpDataTimeout); 

					if(dataReturned == kTCP_TimeOutErr) 
					{
						error = errorDataTimeout;
					} 
					else if (dataReturned < kTCP_NoError) // Won't accept for anything "less than" no errors.
					{
						// Error during receive
						error = errorDataReceive;
					} 
					else if (dataReturned != dataExpected) 
					{
						error = errorDataTimeout;
						dataLost += dataReturned;
					}
					else 
					{
						*dataReceived += dataReturned;
					}
				}
			
				if (error) break;
			
				dataExpected = data->header.length * sizeof(uint) - sizeof(Event_Header);
			
				if (data->header.length < sizeof(Event_Header)) 
				{
					error = errorDataLength;		// Reported size of packet is smaller than a header!
				} 
				else if (dataExpected > (MAX_EVENT_DATA * sizeof (ushort)))
				{
					error = errorDataLength;	// Reported size of waveform is too large!
				}
			
				if (error) break;
				
				// Fetch Event Data
				dataReturned = ClientTCPRead(tcpDataHandle, (uint*)(&(data->waveform)), dataExpected, tcpDataTimeout); 
				if(dataReturned == kTCP_TimeOutErr) 
				{
					error = errorDataTimeout;
				} 
				else if (dataReturned < kTCP_NoError) // Won't accept for anything "less than" no errors.
				{
					// Error during receive
					error = errorDataReceive;
				} 
				else if (dataReturned != dataExpected) 
				{
					error = errorDataTimeout;
					dataLost += dataReturned;
				}
				else 
				{
					*dataReceived += dataReturned;
				}
			#endif
		break;

		case commUSB:
			
			dataSynced		= 0;
			dataExpected	= 4;
			dataLost		= 0;
			*dataReceived	= 0;
			
			do {
				// Synchronize with data stream by searching for 0xAAAAAAAA
				// NOTE: This code assumes misalignments are a complete uint (4 bytes)
				// If the device somehow gets off by 1-3 bytes, it will get stuck here! 
				ftStatus = FT_Read(ftDataDev[deviceNum], dataBuff, dataExpected, (LPDWORD)(&dataReturned));
				if (ftStatus != FT_OK) {
					// Error during receive
					error = errorDataReceive;
				} else if (dataReturned != dataExpected) {
					// Timeout expired - may be only way to catch certain misalignments
					error = errorDataTimeout;
					dataLost += dataReturned;
				} else {
					// Check for start of event
					value = *(uint*)data;
					if (value == 0xAAAAAAAA) {
						dataSynced = 1;
						*dataReceived += dataReturned;
					} else {
						// Throw this data away and increment counter
						dataLost += dataReturned;
					}
				}
				
				if (error) break;

		#ifdef ANL
				LogData(dataBuff, dataReturned);
		#endif
		
			} while (dataSynced == 0);

		#ifdef ANL
			LogComment("SYNCED: %d \n", logEvent);
		#endif
	
			if (error) break;
			
			// Data stream now synced
			// First DWORD of event header (0xAAAAAAAA) has already been read
			dataBuff = (LPVOID*)((uint64_t)data + sizeof(uint));	// Advance data pointer past 0xAAAAAAAA
			dataExpected = sizeof(Event_Header) - sizeof(uint);	// Size of remaining header

			// Fetch rest of Event Header
			ftStatus = FT_Read(ftDataDev[deviceNum], dataBuff, dataExpected, (LPDWORD)(&dataReturned));
		#ifdef ANL
			LogData(dataBuff, dataReturned);
		#endif
			*dataReceived += dataReturned;
			if (ftStatus != FT_OK) {
				// Error during receive
				error = errorDataReceive;
			} else if (dataReturned != dataExpected) {
				// Timeout expired
				error = errorDataTimeout;
				dataLost += dataReturned;
			} else {
				*dataReceived += dataReturned;
				// Prepare to read Event Data
				dataBuff = (LPVOID*)((uint64_t)data + sizeof(Event_Header));// Location to store waveform
				dataExpected = (event->header.length) * sizeof(uint);	// Size of packet in bytes
				if (dataExpected < sizeof(Event_Header)) {
					error = errorDataLength;		// Reported size of packet is smaller than a header!
				} else {
					dataExpected = dataExpected - sizeof(Event_Header);	// Size of waveform in bytes
					if (dataExpected > (MAX_EVENT_DATA * sizeof (ushort))) {
						error = errorDataLength;	// Reported size of waveform is too large!
					}
				}
			}

		#ifdef ANL
			LogComment("WAVE: %d \n", logEvent);
		#endif
		
			if (error) break;

			// Fetch Event Data
			ftStatus = FT_Read(ftDataDev[deviceNum], dataBuff, dataExpected, (LPDWORD)(&dataReturned));
			if (ftStatus != FT_OK) {
				// Error during receive
				error = errorDataReceive;
			} else if (dataReturned != dataExpected) {
				error = errorDataTimeout;
				dataLost += dataReturned;
			}
			else {
				*dataReceived += dataReturned;
			}

		#ifdef ANL
			LogData(dataBuff, dataReturned);
		#endif
		break;
	}

#ifdef ANL
	logEvent++;

	// Restore previous state of error popups in Debug Mode
	SetBreakOnLibraryErrors(state);
#endif
	
	return error;
}

uint DeviceLostData (void)
{
	return dataLost;
}

//==============================================================================
// Command Functions
//==============================================================================

int DeviceRead (uint address, uint* value, int deviceNum)
{
 	int 		error = 0;
	Ctrl_Packet	rx;
	Ctrl_Packet	tx;
	int			rxSizeExpected = 0;

	tx.header.length	= sizeof(Ctrl_Header);
	tx.header.address	= address;
	tx.header.command	= cmdRead;
	tx.header.size		= 1;
	tx.header.status	= statusNoError;
	rxSizeExpected		= sizeof(Ctrl_Header) + sizeof(uint);
	
	error = SendReceive(&tx, &rx, retryOn, deviceNum);
	if (error == 0) {
		if(tx.header.address != rx.header.address)
		{
			error = errorCommReceive;
		}
	}
	
	if (error == 0) {
		// No Error, return data
		*value = rx.data[0];
	} else {
		// Error, return zero
		*value = 0;
#ifdef ERROR_MESSAGE
	#if defined(ANL)
		sprintf(errString,"Error=%d, Address=0x%08X",error, address);
		MessagePopup("DeviceRead", errString);
	#else
		std::cout << "DeviceRead Error = " << error << "at" << std::hex  << address << std::endl;
	#endif
#endif
	}
	
	return error;
}

int DeviceReadMask (uint address, uint mask, uint* value, int deviceNum)
{
 	int 		error = 0;
	Ctrl_Packet	rx;
	Ctrl_Packet	tx;
	int			rxSizeExpected = 0;

	tx.header.length	= sizeof(Ctrl_Header) + sizeof(uint);
	tx.header.address	= address;
	tx.header.command	= cmdReadMask;
	tx.header.size		= 1;
	tx.header.status	= statusNoError;
	tx.data[0]			= mask;
	rxSizeExpected		= sizeof(Ctrl_Header) + sizeof(uint);
	
	error = SendReceive(&tx, &rx, retryOn, deviceNum);
	if (error == 0) {
		if(tx.header.address != rx.header.address)
		{
			error = errorCommReceive;
		}
	}
	
	if (error == 0) {
		// No Error, return data
		*value = rx.data[0];
	} else {
		// Error, return zero
		*value = 0;
#ifdef ERROR_MESSAGE
	#if defined(ANL)
		sprintf(errString,"Error=%d, Address=0x%08X",error, address);
		MessagePopup("DeviceReadMask", errString);
	#else
		std::cout << "DeviceReadMask Error = " << error << "at" << std::hex  << address << std::endl;
	#endif
#endif
	}
	
	return error;
}

int DeviceWrite (uint address, uint value, int deviceNum)
{
	int 		error = 0;
	Ctrl_Packet	rx;
	Ctrl_Packet	tx;
	int			rxSizeExpected = 0;
	
	tx.header.length	= sizeof(Ctrl_Header) + sizeof(uint);
	tx.header.address	= address;
	tx.header.command	= cmdWrite;
	tx.header.size		= 1;
	tx.header.status	= statusNoError;
	tx.data[0]			= value;
	rxSizeExpected		= sizeof(Ctrl_Header);

	error = SendReceive(&tx, &rx, retryOff, deviceNum);
	if (error == 0) {
		if(tx.header.address != rx.header.address)
		{
			error = errorCommSend;
		}
	}
	
	if (error != 0) {
#ifdef ERROR_MESSAGE
	#if defined(ANL)
		sprintf(errString,"Error=%d, Address=0x%08X",error, address);
		MessagePopup("DeviceWrite", errString);
	#else
		std::cout << "DeviceWrite Error = " << error << "at" << std::hex  << address << std::endl;
	#endif
#endif
	}

	return error;
}

int DeviceWriteMask (uint address, uint mask, uint value, int deviceNum)
{
	int 		error = 0;
	Ctrl_Packet	rx;
	Ctrl_Packet	tx;
	int			rxSizeExpected = 0;

	tx.header.length	= sizeof(Ctrl_Header) + (sizeof(uint) * 2);
	tx.header.address	= address;
	tx.header.command	= cmdWriteMask;
	tx.header.size		= 1;
	tx.header.status	= statusNoError;
	tx.data[0]			= mask;
	tx.data[1]			= value;
	rxSizeExpected		= sizeof(Ctrl_Header) + sizeof(uint); 
	
	error = SendReceive(&tx, &rx, retryOff, deviceNum);
	if (error == 0) {
		if(tx.header.address != rx.header.address)
		{
			error = errorCommSend;
		}					 
	}
	
	if (error != 0) {
#ifdef ERROR_MESSAGE
	#if defined(ANL)
		sprintf(errString,"Error=%d, Address=0x%08X",error, address);
		MessagePopup("DeviceWriteMask", errString);
	#else
		std::cout << "DeviceWriteMask Error = " << error << "at" << std::hex  << address << std::endl;
	#endif
#endif
	}

	return error;
}

int DeviceSet (uint address, uint mask, int deviceNum)
{
  return DeviceWriteMask(address, mask, 0xFFFFFFFF, deviceNum);
}

int DeviceClear (uint address, uint mask, int deviceNum)
{
  return DeviceWriteMask(address, mask, 0x00000000, deviceNum);
}

int DeviceArrayRead (uint address, uint size, uint* data, int deviceNum)
{
	uint 		i = 0;
 	int 		error = 0;
	Ctrl_Packet	rx;
	Ctrl_Packet	tx;
	int			rxSizeExpected = 0;

	tx.header.length	= sizeof(Ctrl_Header);
	tx.header.address	= address;
	tx.header.command	= cmdArrayRead;
	tx.header.size		= size;
	tx.header.status	= statusNoError;
	rxSizeExpected		= sizeof(Ctrl_Header) + (sizeof(uint) * size);

	error = SendReceive(&tx, &rx, retryOn, deviceNum);
	if (error == 0) {
		if(tx.header.address != rx.header.address)
		{
			error = errorCommReceive;
		}
		// No Error, return data
		for (i = 0; i < size; i++) {
			data[i] = rx.data[i];
		}
	} else {
		// Error, return zeros
		for (i = 0; i < size; i++) {
			data[i] = 0;
		}
	}
	if(error) 
	{
	#ifdef ERROR_MESSAGE
		#if defined(ANL)
			sprintf(errString,"Error=%d, Address=0x%08X",error, address);
			MessagePopup("DeviceArrayRead", errString);
		#else
			std::cout << "DeviceArrayRead Error = " << error << "at" << std::hex  << address << std::endl;
		#endif
	#endif
	}
	
	return error;
}

int DeviceArrayWrite (uint address, uint size, uint* data, int deviceNum)
{
	uint 		i = 0;
 	int 		error = 0;
	Ctrl_Packet	rx;
	Ctrl_Packet	tx;
	int			rxSizeExpected = 0;

	tx.header.length	= sizeof(Ctrl_Header) + (sizeof(uint) * size);
	tx.header.address	= address;
	tx.header.command	= cmdArrayWrite;
	tx.header.size		= size;
	tx.header.status	= statusNoError;
	rxSizeExpected		= sizeof(Ctrl_Header);
	
	for (i = 0; i < size; i++) {
		tx.data[i] = data[i];
	}

	error = SendReceive(&tx, &rx, retryOff, deviceNum);
	if (error == 0) {
		if(tx.header.address != rx.header.address)
		{
			error = errorCommSend;
		}
	}
	
	if (error != 0) {
#ifdef ERROR_MESSAGE
	#if defined(ANL)
		sprintf(errString,"Error=%d, Address=0x%08X",error, address);
		MessagePopup("DeviceArrayWrite", errString);
	#else
		std::cout << "DeviceArrayWrite Error = " << error << "at" << std::hex  << address << std::endl;
	#endif
#endif
	}
	
	return error;
}

int DeviceFifoRead (uint address, uint size, uint* data, int deviceNum)
{
	uint			i = 0;
 	int			error = 0;
	Ctrl_Packet	rx;
	Ctrl_Packet	tx;
	int			rxSizeExpected = 0;

	tx.header.length	= sizeof(Ctrl_Header);
	tx.header.address	= address;
	tx.header.command	= cmdFifoRead;
	tx.header.size		= size;
	tx.header.status	= statusNoError;
	rxSizeExpected		= sizeof(Ctrl_Header) + (sizeof(uint) * size);

	error = SendReceive(&tx, &rx, retryOff, deviceNum);
	if (error == 0) {
		if(tx.header.address != rx.header.address)
		{
			error = errorCommReceive;
		}
	}
		
	if (error == 0) {
		// No Error, return data
		for (i = 0; i < size; i++) {
			data[i] = rx.data[i];
		}
	} else {
		// Error, return zeros
		for (i = 0; i < size; i++) {
			data[i] = 0;
		}
#ifdef ERROR_MESSAGE
	#if defined(ANL)
		sprintf(errString,"Error=%d, Address=0x%08X",error, address);
		MessagePopup("DeviceFifoRead", errString);
	#else
		std::cout << "DeviceFifoRead Error = " << error << "at" << std::hex  << address << std::endl;
	#endif
#endif
	}

	return error;
}

int DeviceFifoWrite (uint address, uint size, uint* data, int deviceNum)
{
	uint			i = 0;
	int			error = 0;
	Ctrl_Packet	rx;
	Ctrl_Packet	tx;
	int			rxSizeExpected = 0;

	tx.header.length	= sizeof(Ctrl_Header) + (sizeof(uint) * size);
	tx.header.address	= address;
	tx.header.command	= cmdFifoWrite;
	tx.header.size		= size;
	tx.header.status	= statusNoError;
	rxSizeExpected		= sizeof(Ctrl_Header); 
	
	for (i = 0; i < size; i++) {
		tx.data[i] = data[i];
	}

	error = SendReceive(&tx, &rx, retryOff, deviceNum);
	if (error == 0) {
		if(tx.header.address != rx.header.address)
		{
			error = errorCommSend;
		}
	}
		
	if (error != 0) {
#ifdef ERROR_MESSAGE
	#if defined(ANL)
		sprintf(errString,"Error=%d, Address=0x%08X",error, address);
		MessagePopup("DeviceFifoWrite", errString);
	#else
		std::cout << "DeviceFifoWrite Error = " << error << "at" << std::hex  << address << std::endl;
	#endif
#endif
	}
	
	return error;
}

int DeviceNVWrite (uint address, uint value, int deviceNum)
{
	int 		error = 0;
	Ctrl_Packet	rx;
	Ctrl_Packet	tx;
	int			rxSizeExpected = 0;
	
	tx.header.length	= sizeof(Ctrl_Header) + sizeof(uint);
	tx.header.address	= address;
	tx.header.command	= cmdNVWrite;
	tx.header.size		= 1;
	tx.header.status	= statusNoError;
	tx.data[0]			= value;
	rxSizeExpected		= sizeof(Ctrl_Header);

	error = SendReceive(&tx, &rx, retryOff, deviceNum);
	if (error == 0) {
		if(tx.header.address != rx.header.address)
		{
			error = errorCommSend;
		}
	}
		
	if (error != 0) {
#ifdef ERROR_MESSAGE
	#if defined(ANL)
		sprintf(errString,"Error=%d, Address=0x%08X",error, address);
		MessagePopup("DeviceNVWrite", errString);
	#else
		std::cout << "DeviceNVWrite Error = " << error << "at" << std::hex  << address << std::endl;
	#endif
#endif
	}

	return error;
}

int DeviceNVArrayWrite (uint address, uint size, uint* data, int deviceNum)
{
	uint 		i = 0;
 	int 		error = 0;
	Ctrl_Packet	rx;
	Ctrl_Packet	tx;
	int			rxSizeExpected = 0;

	tx.header.length	= sizeof(Ctrl_Header) + (sizeof(uint) * size);
	tx.header.address	= address;
	tx.header.command	= cmdNVArrayWrite;
	tx.header.size		= size;
	tx.header.status	= statusNoError;
	rxSizeExpected		= sizeof(Ctrl_Header);
	
	for (i = 0; i < size; i++) {
		tx.data[i] = data[i];
	}

	error = SendReceive(&tx, &rx, retryOff, deviceNum);
	if (error == 0) {
		if(tx.header.address != rx.header.address)
		{
			error = errorCommSend;
		}
	}
		
	if (error != 0) {
#ifdef ERROR_MESSAGE
	#if defined(ANL)
		sprintf(errString,"Error=%d, Address=0x%08X",error, address);
		MessagePopup("DeviceNVArrayWrite", errString);
	#else
		std::cout << "DeviceNVArrayWrite Error = " << error << "at" << std::hex  << address << std::endl;
	#endif
#endif
	}
	
	return error;
}

int DeviceNVEraseSector (uint address,int deviceNum)
{
 	int 		error = 0;
	Ctrl_Packet	rx;
	Ctrl_Packet	tx;
	int			rxSizeExpected = 0;

	tx.header.length	= sizeof(Ctrl_Header);
	tx.header.address	= address;
	tx.header.command	= cmdNVEraseSector;
	tx.header.size		= 0;
	tx.header.status	= statusNoError;
	rxSizeExpected		= sizeof(Ctrl_Header);
	
	error = SendReceive(&tx, &rx, retryOff, deviceNum);
	if (error == 0) {
		if(tx.header.address != rx.header.address)
		{
			error = errorCommSend;
		}
	}
	
	if (error == 0) {
		// No Error, return data
	} else {
#ifdef ERROR_MESSAGE
	#if defined(ANL)
		sprintf(errString,"Error=%d, Address=0x%08X",error, address);
		MessagePopup("DeviceNVEraseSector", errString);
	#else
		std::cout << "DeviceNVEraseSector Error = " << error << "at" << std::hex  << address << std::endl;
	#endif
#endif
	}
	
	return error;
}

int DeviceNVEraseBlock (uint address, int deviceNum)
{
 	int 		error = 0;
	Ctrl_Packet	rx;
	Ctrl_Packet	tx;
	int			rxSizeExpected = 0;

	tx.header.length	= sizeof(Ctrl_Header);
	tx.header.address	= address;
	tx.header.command	= cmdNVEraseBlock;
	tx.header.size		= 0;
	tx.header.status	= statusNoError;
	rxSizeExpected		= sizeof(Ctrl_Header);
	
	error = SendReceive(&tx, &rx, retryOff, deviceNum);
	if (error == 0) {
		if(tx.header.address != rx.header.address)
		{
			error = errorCommSend;
		}
	}
		
	if (error == 0) {
		// No Error, return data
	} else {
#ifdef ERROR_MESSAGE
	#if defined(ANL)
		sprintf(errString,"Error=%d, Address=0x%08X",error, address);
		MessagePopup("DeviceNVEraseBlock", errString);
	#else
		std::cout << "DeviceNVEraseBlock Error = " << error << "at" << std::hex  << address << std::endl;
	#endif
#endif
	}
	
	return error;
}

int DeviceNVEraseChip (uint address, int deviceNum)
{
 	int 		error = 0;
	Ctrl_Packet	rx;
	Ctrl_Packet	tx;
	int			rxSizeExpected = 0;

	tx.header.length	= sizeof(Ctrl_Header);
	tx.header.address	= address;
	tx.header.command	= cmdNVEraseChip;
	tx.header.size		= 0;
	tx.header.status	= statusNoError;
	rxSizeExpected		= sizeof(Ctrl_Header);
	
	error = SendReceive(&tx, &rx, retryOff, deviceNum);
	if (error == 0) {
		if(tx.header.address != rx.header.address)
		{
			error = errorCommSend;
		}
	}
	
	if (error == 0) {
		// No Error, return data
	} else {
#ifdef ERROR_MESSAGE
	#if defined(ANL)
		sprintf(errString,"Error=%d, Address=0x%08X",error, address);
		MessagePopup("DeviceNVEraseChip", errString);
	#else
		std::cout << "DeviceNVEraseChip Error = " << error << "at" << std::hex  << address << std::endl;
	#endif
#endif
	}
	
	return error;
}

//==============================================================================
// Support Functions
//==============================================================================

int SendReceive (Ctrl_Packet* tx, Ctrl_Packet* rx, uint retry, int deviceNum)
{
	int error = 0;
	#ifdef ANL_TCP
		int rxSize;
	#else
	#ifdef ANL_UDP
		int rxSize;
	#endif
	#endif
		
	#ifdef ANL	
		int	state = 0;
	#endif

	#ifdef ANL	
	// Disable error popups so timeouts can occur in Debug Mode
	state = SetBreakOnLibraryErrors(0);
	#endif

	switch (savedCommType) {
		case commNone:
			// This option only for debugging a "connected" device
			break;
		case commTCP:
			#ifdef ANL_TCP
				error = ClientTCPWrite(tcpControlHandle, tx, tx->header.length, tcpControlTimeout);
				if (error < kTCP_NoError) {
					error = errorCommSend;
					break;
				}
		
				// The first word is the total length of the packet.
				rxSize = ClientTCPRead(tcpControlHandle, &(rx->header.length), sizeof(uint), tcpControlTimeout);
				if (rxSize == 0) {
					error = errorCommReceiveZero;
				}
				else if (error < kTCP_NoError) {
					error = errorCommReceive;
					break;
				}
				else if ((rx->header.length > sizeof(Ctrl_Packet)) || (rx->header.length < sizeof(Ctrl_Header)))  {
					error = errorCommReceive;
					break;
				}
			
				if(error >= kTCP_NoError)
				{
					// Read the rest of the data packet into the allocated receive buffer.
					rxSize = ClientTCPRead(tcpControlHandle, &(((uint*)(rx))[1]), rx->header.length - sizeof(uint), tcpControlTimeout);
					if (rxSize == 0) {
						error = errorCommReceiveZero;
					} else if (error < kTCP_NoError) {   
						error = errorCommReceive;
					} else {
						error = errorNoError;
					}
				}
			#endif
			break;
			
		case commUDP:
			#ifdef ANL_UDP
				error = UDPWrite(udpControlChannel, ControlPort0, DeviceIP, tx, tx->header.length);
				if (error) {
					error = errorCommSend;
					break;
				}
			
				// Wait for a data packet to arrive:
				// Inputs to UDPRead():
				//    UDP channel number
				//    Buffer to store read data (NULL = check size only)
				//    Size of buffer for storing read data
				//    Timeout to wait for response (0 = return immediate, time in ms otherwise)
				// Outputs from UDPRead():
				//    Port of received packet (0 = discard)
				//    IP Address of received packet (0 = discard)
				//    Returns size of received packet
			
				do {
				  rxSize = UDPRead(udpControlChannel, &(rx->header.length), sizeof(uint), 100, 0, 0);
					// Perform idle tasks here
					// Check for press of a future abort button here
				} while (rxSize < 4);
			
				do {
				  rxSize = UDPRead(udpControlChannel, NULL, 0, 100, 0, 0);
					// Perform idle tasks here
					// Check for press of a future abort button here
				} while (rxSize < (rx->header.length - sizeof(uint)));
			
				// Read the data packet into the allocated receive buffer.
				rxSize = UDPRead(udpControlChannel, &(((uint*)(rx))[1]), rx->header.length - sizeof(uint), 0, 0, 0);
				if (rxSize == 0) {
					error = errorCommReceiveZero;
				} else if (rxSize < 0) {
					error = errorCommReceive;
				} else {
					error = errorNoError;
				}
			#endif
			break;
		case commUSB:
			error = SendReceiveUSB(tx, rx, retry, deviceNum);
			break;
		default:
			error = errorCommType;
			break;
	}

	
	#ifdef ANL
	// Restore previous state of error popups in Debug Mode
	SetBreakOnLibraryErrors(state);
	#endif
	
	return error;
}

int SendReceiveUSB (Ctrl_Packet* tx, Ctrl_Packet* rx, uint retry, int deviceNum)
{
	int error = 0;
	
	error = DevicePurgeComm(deviceNum);
	if (error) {
		// Error during purge, return error packet
		RxErrorPacket(statusSendError, rx);
	}
	
	if (error == 0) {
		error = SendUSB(tx, deviceNum);
		#ifndef ANL
		// Insert small delay between send and receive on Linux
		usleep(postSendDelay);
		#endif
	}

	if (error == 0) {
		error = ReceiveUSB(rx, deviceNum);
		#ifndef ANL
		// Insert small delay between transactions on Linux
		usleep(postReceiveDelay);
		#endif
	}
	
	if ((error == errorCommReceiveZero) || (error == errorCommReceiveTimeout)) {
		if (retry == retryOn) {
			#ifdef ERROR_MESSAGE
				#if defined(ANL)
					sprintf(errString, "Error=%d, Address=0x%08X, Retrying", error, tx->header.address);
					MessagePopup("SendReceive", errString);
				#else
					std::cout << "SendReceive Error = " << error << "at" << std::hex  << tx->header.address << ", Retrying" << std::endl;
				#endif
			#endif

			error = SendUSB(tx, deviceNum);
			#ifndef ANL
			// Insert small delay between send and receive on Linux
			usleep(postSendDelay);
			#endif
			
			if (error == 0) {
				error = ReceiveUSB(rx, deviceNum);
				#ifndef ANL
				// Insert small delay between transactions of Linux
				usleep(postReceiveDelay);
				#endif
			}
		}
	}
	
	return error;
}

int SendUSB (Ctrl_Packet* tx, int deviceNum)
{
	int 		error = 0;
	uint			txSizeWritten = 0;
	FT_STATUS	ftStatus;
	
	// Send TX data over FTDI control path
	ftStatus = FT_Write(ftControlDev[deviceNum], (char*)tx, tx->header.length, (LPDWORD)(&txSizeWritten));
	if (ftStatus != FT_OK) {
		// Error during send, return error packet
		error = errorCommSend;
	} else if (txSizeWritten == 0) {
		// Timeout after sending nothing, return error packet
		// BUGBUG: In this case, we could probably retry the send
		error = errorCommSendZero;
	} else if (txSizeWritten != tx->header.length) {
		// Timeout after sending less than expected
		// BUGBUG: In this case, the if the device received enough of the packet
		// it would still respond.  So long as the call to FT_Purge works, we
		// shouldn't get out of sync.
		error = errorCommSendTimeout;
	}
	
	return error;
}

int ReceiveUSB (Ctrl_Packet* rx, int deviceNum)
{
	int			error = 0;
	uint			rxSizeReturned = 0;
	FT_STATUS	ftStatus;
	
	// Request RX data over FTDI control path
	ftStatus = FT_Read(ftControlDev[deviceNum], (LPVOID)&(rx->header.length), sizeof(uint), (LPDWORD)(&rxSizeReturned));
	if (ftStatus != FT_OK) {
		// Error during receive, return error packet
		error = errorCommReceive;
		RxErrorPacket(statusReceiveError, rx);
	} else if (rxSizeReturned == 0) {
		// Timeout after receiving nothing, return error packet
		error = errorCommReceiveZero;
		RxErrorPacket(statusTimeoutError, rx);
	} else if (rxSizeReturned != sizeof(uint)) {
		// Timeout after receiving less than expected
		// Most likely, device returned an error packet (just a header)
		error = errorCommReceiveTimeout;
	} else {
		// Received expected amount of data
	
		ftStatus = FT_Read(ftControlDev[deviceNum], (LPVOID)&(((uint*)(rx))[1]), rx->header.length - sizeof(uint), (LPDWORD)(&rxSizeReturned));
		if (ftStatus != FT_OK) {
			// Error during receive, return error packet
			error = errorCommReceive;
			RxErrorPacket(statusReceiveError, rx);
		} else if (rxSizeReturned == 0) {
			// Timeout after receiving nothing, return error packet
			error = errorCommReceiveZero;
			RxErrorPacket(statusTimeoutError, rx);
		} else if (rxSizeReturned != rx->header.length - sizeof(uint)) {
			// Timeout after receiving less than expected
			// Most likely, device returned an error packet (just a header)
			error = errorCommReceiveTimeout;
		} else {
			// Received expected amount of data
		}
	}
	
	return error;
}

void RxErrorPacket (uint status, Ctrl_Packet* rx) {
	int i = 0;
	
	rx->header.length	= sizeof(Ctrl_Header);
	rx->header.address	= 0;
	rx->header.command	= cmdNone;
	rx->header.size		= 0;
	rx->header.status	= status;
	for (i = 0; i < MAX_CTRL_DATA; i++) {
		rx->data[i] = 0;
	}	  
	
	return;
}

//==============================================================================
// Script Functions
//==============================================================================

double DeviceSetupTime (void)
{
	return elapsedTime;
}

int DeviceSetup (int deviceNum)
{
	// Setting up some constants to use during initialization
	const uint	module_id		= 0xABC;	// This value is reported in the event header
	const uint	channel_control[MAX_CHANNELS] = 
	//	Channel Control Bit Descriptions
	//	31		cfd_enable
	//	30		pileup_waveform_only
	//	26		pileup_extend_enable
	//	25:24	event_extend_mode
	//	23		disc_counter_mode
	//	22		ahit_counter_mode 
	//	21		ACCEPTED_EVENT_COUNTER_MODE
	//	20		dropped_event_counter_mode
	//	15:14	external_disc_flag_sel
	//	13:12	external_disc_mode
	//	11		negative edge trigger enable
	//	10		positive edge trigger enable
	//	6:4		This sets the timestamp trigger rate (source of external_disc_flag_in(3) into channel logic)
	//	2		Not pileup_disable
	//	1		Trigger Mode
	//	0		channel enable
	//
	//	Channel Control Examples
	//	0x00000000,		// disable channel #
	//	0x80F00001,		// enable channel # but do not enable any triggers (CFD Enabled)
	//	0x80F00401,		// configure channel # in positive self-trigger mode (CFD Enabled)
	//	0x80F00801,		// configure channel # in negative self-trigger mode (CFD Enabled)
	//	0x80F00C01,		// configure channel # in positive and negative self-trigger mode (CFD Enabled)
	//	0x00F06001,		// configure channel # in external trigger mode.
	//	0x00F0E051,		// configure channel # in a slow timestamp triggered mode (8.941Hz)
	//	0x00F0E061,		// configure channel # in a very slow timestamp triggered mode (1.118Hz)
	{
		0x00F0E061,		// configure channel #0 in a slow timestamp triggered mode
		0x00000000,		// disable channel #1
		0x00000000,		// disable channel #2
		0x00000000,		// disable channel #3
		0x00000000,		// disable channel #4
		0x00000000,		// disable channel #5
		0x00000000,		// disable channel #6
		0x00000000,		// disable channel #7
		0x00000000,		// disable channel #8
		0x00000000,		// disable channel #9
		0x00000000,		// disable channel #10
		0x00000000,		// disable channel #11
	};
	const uint	led_threshold		= 25;	
	const uint	cfd_fraction		= 0x1800;
	const uint	readout_pretrigger	= 100;	
	const uint	event_packet_length	= 2046;	
	const uint	p_window			= 0;	
	const uint	i2_window			= 500;
	const uint	m1_window			= 10;	
	const uint	m2_window			= 10;	
	const uint	d_window			= 20;
	const uint  i1_window			= 500;
	const uint	disc_width			= 10;
	const uint	baseline_start		= 0x0000;
	const uint	baseline_delay		= 5;

	int i = 0;
	uint data[MAX_CHANNELS];

	// Check timestamp at start of DeviceSetup()
	DeviceRead(lbneReg.live_timestamp_lsb, &startTime, deviceNum);

	// This script of register writes sets up the digitizer for basic real event operation
	// Comments next to each register are excerpts from the VHDL or C code
	// ALL existing registers are shown here however many are commented out because they are
	// status registers or simply don't need to be modified
	// The script runs through the registers numerically (increasing addresses)
	// Therefore, it is assumed DeviceStopReset() has been called so these changes will not
	// cause crazy things to happen along the way

	//	Registers in the Zynq ARM											//	Address		Default Value	Read Mask		Write Mask		Code Name

	//	Registers in the Zynq FPGA (Comm)									//	Address		Default Value	Read Mask		Write Mask		Code Name
	//	DeviceWrite(lbneReg.reg_event_data_interface_sel, 	0x00000000, deviceNum);	//	X"020",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_event_data_interface_sel
	//	DeviceWrite(lbneReg.codeErrCounts[0], 		Read_Only			, deviceNum);	//	X"100",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_code_err_counts(0)
	//	DeviceWrite(lbneReg.codeErrCounts[1], 		Read_Only			, deviceNum);	//	X"104",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_code_err_counts(1)
	//	DeviceWrite(lbneReg.codeErrCounts[2], 		Read_Only			, deviceNum);	//	X"108",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_code_err_counts(2)
	//	DeviceWrite(lbneReg.codeErrCounts[3], 		Read_Only			, deviceNum);	//	X"10C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_code_err_counts(3)
	//	DeviceWrite(lbneReg.codeErrCounts[4], 		Read_Only			, deviceNum);	//	X"110",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_code_err_counts(4)
	//	DeviceWrite(lbneReg.dispErrCounts[0], 		Read_Only			, deviceNum);	//	X"120",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_disp_err_counts(0)
	//	DeviceWrite(lbneReg.dispErrCounts[1], 		Read_Only			, deviceNum);	//	X"124",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_disp_err_counts(1)
	//	DeviceWrite(lbneReg.dispErrCounts[2], 		Read_Only			, deviceNum);	//	X"128",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_disp_err_counts(2)
	//	DeviceWrite(lbneReg.dispErrCounts[3], 		Read_Only			, deviceNum);	//	X"12C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_disp_err_counts(3)
	//	DeviceWrite(lbneReg.dispErrCounts[4], 		Read_Only			, deviceNum);	//	X"130",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_disp_err_counts(4)
	//	DeviceWrite(lbneReg.link_rx_status, 		Read_Only			, deviceNum);	//	X"140",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_link_rx_status
	//	DeviceWrite(lbneReg.eventDataControl, 		In DeviceStart/Stop	, deviceNum);	//	X"144",		X"0000001F",	X"FFFFFFFF",	X"0013001F",	reg_event_data_control
	//	DeviceWrite(lbneReg.eventDataPhaseControl, 	Write_Only			, deviceNum);	//	X"148",		X"00000000",	X"00000000",	X"00000003",	reg_event_data_phase_control
	//	DeviceWrite(lbneReg.eventDataPhaseStatus, 	Read_Only			, deviceNum);	//	X"14C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_event_data_phase_status
	//	DeviceWrite(lbneReg.c2c_master_status,		Read_Only			, deviceNum);	//	X"150",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_c2c_master_status
	        DeviceWrite(lbneReg.c2c_control,			0x00000007			, deviceNum);	//	X"154",		X"00000007",	X"FFFFFFFF",	X"00000007",	reg_c2c_control
		DeviceWrite(lbneReg.c2c_master_intr_control,0x00000000			, deviceNum);	//	X"158",		X"00000000",	X"FFFFFFFF",	X"0000000F",	reg_c2c_master_intr_control
	//	DeviceWrite(lbneReg.dspStatus, 				Read_Only			, deviceNum);	//	X"160",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_dsp_status
	//	DeviceWrite(lbneReg.comm_clock_status, 		Read_Only			, deviceNum);	//	X"170",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_comm_clock_status
		DeviceWrite(lbneReg.comm_clock_control,		0x00000001			, deviceNum);	//	X"174",		X"00000001",	X"FFFFFFFF",	X"00000001",	reg_comm_clock_control
		DeviceWrite(lbneReg.comm_led_config, 		0x00000000			, deviceNum);	//	X"180",		X"00000000",	X"FFFFFFFF",	X"00000013",	reg_comm_led_config
		DeviceWrite(lbneReg.comm_led_input, 		0x00000000			, deviceNum);	//	X"184",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_comm_led_input
	//	DeviceWrite(lbneReg.eventDataStatus, 		Read_Only			, deviceNum);	//	X"190",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_event_data_status

	//	DeviceWrite(lbneReg.qi_dac_control,			Do_After_Config!	, deviceNum);	//	X"200",		X"00000000",	X"00000000",	X"00000001",	reg_qi_dac_control
		DeviceWrite(lbneReg.qi_dac_config,			0x00000000			, deviceNum);	//	X"204",		X"00008000",	X"0003FFFF",	X"0003FFFF",	reg_qi_dac_config
		DeviceWrite(lbneReg.qi_dac_control,			0x00000001			, deviceNum);	//	X"200",		X"00000000",	X"00000000",	X"00000001",	reg_qi_dac_control

	//	DeviceWrite(lbneReg.bias_control,			Do_After_Config!	, deviceNum);	//	X"300",		X"00000000",	X"00000000",	X"00000001",	reg_bias_control
	//	DeviceWrite(lbneReg.bias_status,			Read_Only			, deviceNum);	//	X"304",		X"00000000",	X"00000FFF",	X"00000000",	regin_bias_status
		DeviceWrite(lbneReg.bias_config[0],			0x00000000			, deviceNum);	//	X"340",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(0)
		DeviceWrite(lbneReg.bias_config[1],			0x00000000			, deviceNum);	//	X"344",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(1)
		DeviceWrite(lbneReg.bias_config[2],			0x00000000			, deviceNum);	//	X"348",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(2)
		DeviceWrite(lbneReg.bias_config[3],			0x00000000			, deviceNum);	//	X"34C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(3)
		DeviceWrite(lbneReg.bias_config[4],			0x00000000			, deviceNum);	//	X"350",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(4)
		DeviceWrite(lbneReg.bias_config[5],			0x00000000			, deviceNum);	//	X"354",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(5)
		DeviceWrite(lbneReg.bias_config[6],			0x00000000			, deviceNum);	//	X"358",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(6)
		DeviceWrite(lbneReg.bias_config[7],			0x00000000			, deviceNum);	//	X"35C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(7)
		DeviceWrite(lbneReg.bias_config[8],			0x00000000			, deviceNum);	//	X"360",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(8)
		DeviceWrite(lbneReg.bias_config[9],			0x00000000			, deviceNum);	//	X"364",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(9)
		DeviceWrite(lbneReg.bias_config[10],		0x00000000			, deviceNum);	//	X"368",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(10)
		DeviceWrite(lbneReg.bias_config[11],		0x00000000			, deviceNum);	//	X"36C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(11)
	//	DeviceWrite(lbneReg.bias_readback[0],		Read_Only			, deviceNum);	//	X"380",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(0)
	//	DeviceWrite(lbneReg.bias_readback[1],		Read_Only			, deviceNum);	//	X"384",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(1)
	//	DeviceWrite(lbneReg.bias_readback[2],		Read_Only			, deviceNum);	//	X"388",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(2)
	//	DeviceWrite(lbneReg.bias_readback[3],		Read_Only			, deviceNum);	//	X"38C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(3)
	//	DeviceWrite(lbneReg.bias_readback[4],		Read_Only			, deviceNum);	//	X"390",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(4)
	//	DeviceWrite(lbneReg.bias_readback[5],		Read_Only			, deviceNum);	//	X"394",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(5)
	//	DeviceWrite(lbneReg.bias_readback[6],		Read_Only			, deviceNum);	//	X"398",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(6)
	//	DeviceWrite(lbneReg.bias_readback[7],		Read_Only			, deviceNum);	//	X"39C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(7)
	//	DeviceWrite(lbneReg.bias_readback[8],		Read_Only			, deviceNum);	//	X"3A0",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(8)
	//	DeviceWrite(lbneReg.bias_readback[9],		Read_Only			, deviceNum);	//	X"3A4",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(9)
	//	DeviceWrite(lbneReg.bias_readback[10],		Read_Only			, deviceNum);	//	X"3A8",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(10)
	//	DeviceWrite(lbneReg.bias_readback[11],		Read_Only			, deviceNum);	//	X"3AC",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(11)
		DeviceWrite(lbneReg.bias_control,			0x00000001			, deviceNum);	//	X"300",		X"00000000",	X"00000000",	X"00000001",	reg_bias_control

		DeviceWrite(lbneReg.mon_config,				0x0012F000			, deviceNum);	//	X"400",		X"0012F000",	X"00FFFFFF",	X"00FFFFFF",	reg_mon_config
		DeviceWrite(lbneReg.mon_select,				0x00FFFF00			, deviceNum);	//	X"404",		X"00FFFF00",	X"FFFFFFFF",	X"FFFFFFFF",	reg_mon_select
		DeviceWrite(lbneReg.mon_gpio,				0x00000000			, deviceNum);	//	X"408",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_mon_gpio
	//	DeviceWrite(lbneReg.mon_config_readback,	Read_Only			, deviceNum);	//	X"410",		X"00000000",	X"00FFFFFF",	X"00000000",	regin_mon_config_readback
	//	DeviceWrite(lbneReg.mon_select_readback,	Read_Only			, deviceNum);	//	X"414",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_select_readback
	//	DeviceWrite(lbneReg.mon_gpio_readback,		Read_Only			, deviceNum);	//	X"418",		X"00000000",	X"0000FFFF",	X"00000000",	regin_mon_gpio_readback
	//	DeviceWrite(lbneReg.mon_id_readback,		Read_Only			, deviceNum);	//	X"41C",		X"00000000",	X"000000FF",	X"00000000",	regin_mon_id_readback
		DeviceWrite(lbneReg.mon_control,			0x00010001			, deviceNum);	//	X"420",		X"00000000",	X"00010100",	X"00010001",	reg_mon_control & regin_mon_control
	//	DeviceWrite(lbneReg.mon_status,				Read_Only			, deviceNum);	//	X"424",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_status

	//	DeviceWrite(lbneReg.mon_bias[0],			Read_Only			, deviceNum);	//	X"440",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(0)
	//	DeviceWrite(lbneReg.mon_bias[1],			Read_Only			, deviceNum);	//	X"444",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(1)
	//	DeviceWrite(lbneReg.mon_bias[2],			Read_Only			, deviceNum);	//	X"448",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(2)
	//	DeviceWrite(lbneReg.mon_bias[3],			Read_Only			, deviceNum);	//	X"44C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(3)
	//	DeviceWrite(lbneReg.mon_bias[4],			Read_Only			, deviceNum);	//	X"450",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(4)
	//	DeviceWrite(lbneReg.mon_bias[5],			Read_Only			, deviceNum);	//	X"454",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(5)
	//	DeviceWrite(lbneReg.mon_bias[6],			Read_Only			, deviceNum);	//	X"458",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(6)
	//	DeviceWrite(lbneReg.mon_bias[7],			Read_Only			, deviceNum);	//	X"45C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(7)
	//	DeviceWrite(lbneReg.mon_bias[8],			Read_Only			, deviceNum);	//	X"460",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(8)
	//	DeviceWrite(lbneReg.mon_bias[9],			Read_Only			, deviceNum);	//	X"464",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(9)
	//	DeviceWrite(lbneReg.mon_bias[10],			Read_Only			, deviceNum);	//	X"468",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(10)
	//	DeviceWrite(lbneReg.mon_bias[11],			Read_Only			, deviceNum);	//	X"46C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(11)

	//	DeviceWrite(lbneReg.mon_value[0],			Read_Only			, deviceNum);	//	X"480",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(0)
	//	DeviceWrite(lbneReg.mon_value[1],			Read_Only			, deviceNum);	//	X"484",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(1)
	//	DeviceWrite(lbneReg.mon_value[2],			Read_Only			, deviceNum);	//	X"488",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(2)
	//	DeviceWrite(lbneReg.mon_value[3],			Read_Only			, deviceNum);	//	X"48C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(3)
	//	DeviceWrite(lbneReg.mon_value[4],			Read_Only			, deviceNum);	//	X"490",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(4)
	//	DeviceWrite(lbneReg.mon_value[5],			Read_Only			, deviceNum);	//	X"494",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(5)
	//	DeviceWrite(lbneReg.mon_value[6],			Read_Only			, deviceNum);	//	X"498",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(6)
	//	DeviceWrite(lbneReg.mon_value[7],			Read_Only			, deviceNum);	//	X"49C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(7)
	//	DeviceWrite(lbneReg.mon_value[8],			Read_Only			, deviceNum);	//	X"4A0",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(8)

	//	Registers in the Artix FPGA (DSP)									//	Address		Default Value	Read Mask		Write Mask		Code Name
	//	DeviceWrite(lbneReg.board_id, 				Read_Only			, deviceNum);	//	X"000",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_board_id
	//	DeviceWrite(lbneReg.fifo_control, 			In DeviceStart/Stop	, deviceNum);	//	X"004",		X"00000000",	X"0FFFFFFF",	X"08000000",	reg_fifo_control
	//	DeviceWrite(lbneReg.dsp_clock_status, 		Read_Only			, deviceNum);	//	X"020",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dsp_clock_status
		DeviceWrite(lbneReg.module_id,				module_id			, deviceNum);	//	X"024",		X"00000000",	X"00000FFF",	X"00000FFF",	reg_module_id
	//	DeviceWrite(lbneReg.c2c_slave_status, 		Read_Only			, deviceNum);	//	X"030",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_c2c_slave_status
		DeviceWrite(lbneReg.c2c_slave_intr_control,	0x00000000			, deviceNum);	//	X"034",		X"00000000",	X"FFFFFFFF",	X"0000000F",	reg_c2c_slave_intr_control

		for (i = 0; i < MAX_CHANNELS; i++) data[i] = channel_control[i];
		DeviceArrayWrite(lbneReg.channel_control[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.channel_control[0],		channel_control[0]	, deviceNum);	//	X"040",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(0)
	//	DeviceWrite(lbneReg.channel_control[1],		channel_control[1]	, deviceNum);	//	X"044",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(1)
	//	DeviceWrite(lbneReg.channel_control[2],		channel_control[2]	, deviceNum);	//	X"048",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(2)
	//	DeviceWrite(lbneReg.channel_control[3],		channel_control[3]	, deviceNum);	//	X"04C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(3)
	//	DeviceWrite(lbneReg.channel_control[4],		channel_control[4]	, deviceNum);	//	X"050",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(4)
	//	DeviceWrite(lbneReg.channel_control[5],		channel_control[5]	, deviceNum);	//	X"054",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(5)
	//	DeviceWrite(lbneReg.channel_control[6],		channel_control[6]	, deviceNum);	//	X"058",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(6)
	//	DeviceWrite(lbneReg.channel_control[7],		channel_control[7]	, deviceNum);	//	X"05C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(7)
	//	DeviceWrite(lbneReg.channel_control[8],		channel_control[8]	, deviceNum);	//	X"060",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(8)
	//	DeviceWrite(lbneReg.channel_control[9],		channel_control[9]	, deviceNum);	//	X"064",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(9)
	//	DeviceWrite(lbneReg.channel_control[10],	channel_control[10]	, deviceNum);	//	X"068",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(10)
	//	DeviceWrite(lbneReg.channel_control[11],	channel_control[11]	, deviceNum);	//	X"06C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(11)

		for (i = 0; i < MAX_CHANNELS; i++) data[i] = led_threshold;
		DeviceArrayWrite(lbneReg.led_threshold[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.led_threshold[0],		led_threshold		, deviceNum);	//	X"080",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(0)
	//	DeviceWrite(lbneReg.led_threshold[1],		led_threshold		, deviceNum);	//	X"084",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(1)
	//	DeviceWrite(lbneReg.led_threshold[2],		led_threshold		, deviceNum);	//	X"088",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(2)
	//	DeviceWrite(lbneReg.led_threshold[3],		led_threshold		, deviceNum);	//	X"08C",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(3)
	//	DeviceWrite(lbneReg.led_threshold[4],		led_threshold		, deviceNum);	//	X"090",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(4)
	//	DeviceWrite(lbneReg.led_threshold[5],		led_threshold		, deviceNum);	//	X"094",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(5)
	//	DeviceWrite(lbneReg.led_threshold[6],		led_threshold		, deviceNum);	//	X"098",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(6)
	//	DeviceWrite(lbneReg.led_threshold[7],		led_threshold		, deviceNum);	//	X"09C",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(7)
	//	DeviceWrite(lbneReg.led_threshold[8],		led_threshold		, deviceNum);	//	X"0A0",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(8)
	//	DeviceWrite(lbneReg.led_threshold[9],		led_threshold		, deviceNum);	//	X"0A4",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(9)
	//	DeviceWrite(lbneReg.led_threshold[10],		led_threshold		, deviceNum);	//	X"0A8",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(10)
	//	DeviceWrite(lbneReg.led_threshold[11],		led_threshold		, deviceNum);	//	X"0AC",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(11)

		for (i = 0; i < MAX_CHANNELS; i++) data[i] = cfd_fraction;
		DeviceArrayWrite(lbneReg.cfd_parameters[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.cfd_parameters[0],		cfd_fraction		, deviceNum);	//	X"0C0",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(0)
	//	DeviceWrite(lbneReg.cfd_parameters[1],		cfd_fraction		, deviceNum);	//	X"0C4",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(1)
	//	DeviceWrite(lbneReg.cfd_parameters[2],		cfd_fraction		, deviceNum);	//	X"0C8",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(2)
	//	DeviceWrite(lbneReg.cfd_parameters[3],		cfd_fraction		, deviceNum);	//	X"0CC",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(3)
	//	DeviceWrite(lbneReg.cfd_parameters[4],		cfd_fraction		, deviceNum);	//	X"0D0",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(4)
	//	DeviceWrite(lbneReg.cfd_parameters[5],		cfd_fraction		, deviceNum);	//	X"0D4",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(5)
	//	DeviceWrite(lbneReg.cfd_parameters[6],		cfd_fraction		, deviceNum);	//	X"0D8",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(6)
	//	DeviceWrite(lbneReg.cfd_parameters[7],		cfd_fraction		, deviceNum);	//	X"0DC",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(7)
	//	DeviceWrite(lbneReg.cfd_parameters[8],		cfd_fraction		, deviceNum);	//	X"0E0",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(8)
	//	DeviceWrite(lbneReg.cfd_parameters[9],		cfd_fraction		, deviceNum);	//	X"0E4",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(9)
	//	DeviceWrite(lbneReg.cfd_parameters[10],		cfd_fraction		, deviceNum);	//	X"0E8",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(10)
	//	DeviceWrite(lbneReg.cfd_parameters[11],		cfd_fraction		, deviceNum);	//	X"0EC",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(11)

		for (i = 0; i < MAX_CHANNELS; i++) data[i] = readout_pretrigger;
		DeviceArrayWrite(lbneReg.readout_pretrigger[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.readout_pretrigger[0],	readout_pretrigger	, deviceNum);	//	X"100",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(0)
	//	DeviceWrite(lbneReg.readout_pretrigger[1],	readout_pretrigger	, deviceNum);	//	X"104",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(1)
	//	DeviceWrite(lbneReg.readout_pretrigger[2],	readout_pretrigger	, deviceNum);	//	X"108",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(2)
	//	DeviceWrite(lbneReg.readout_pretrigger[3],	readout_pretrigger	, deviceNum);	//	X"10C",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(3)
	//	DeviceWrite(lbneReg.readout_pretrigger[4],	readout_pretrigger	, deviceNum);	//	X"110",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(4)
	//	DeviceWrite(lbneReg.readout_pretrigger[5],	readout_pretrigger	, deviceNum);	//	X"114",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(5)
	//	DeviceWrite(lbneReg.readout_pretrigger[6],	readout_pretrigger	, deviceNum);	//	X"118",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(6)
	//	DeviceWrite(lbneReg.readout_pretrigger[7],	readout_pretrigger	, deviceNum);	//	X"11C",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(7)
	//	DeviceWrite(lbneReg.readout_pretrigger[8],	readout_pretrigger	, deviceNum);	//	X"120",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(8)
	//	DeviceWrite(lbneReg.readout_pretrigger[9],	readout_pretrigger	, deviceNum);	//	X"124",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(9)
	//	DeviceWrite(lbneReg.readout_pretrigger[10],	readout_pretrigger	, deviceNum);	//	X"128",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(10)
	//	DeviceWrite(lbneReg.readout_pretrigger[11],	readout_pretrigger	, deviceNum);	//	X"12C",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(11)

		for (i = 0; i < MAX_CHANNELS; i++) data[i] = event_packet_length;
		DeviceArrayWrite(lbneReg.readout_window[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.readout_window[0],		event_packet_length	, deviceNum);	//	X"140",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(0)
	//	DeviceWrite(lbneReg.readout_window[1],		event_packet_length	, deviceNum);	//	X"144",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(1)
	//	DeviceWrite(lbneReg.readout_window[2],		event_packet_length	, deviceNum);	//	X"148",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(2)
	//	DeviceWrite(lbneReg.readout_window[3],		event_packet_length	, deviceNum);	//	X"14C",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(3)
	//	DeviceWrite(lbneReg.readout_window[4],		event_packet_length	, deviceNum);	//	X"150",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(4)
	//	DeviceWrite(lbneReg.readout_window[5],		event_packet_length	, deviceNum);	//	X"154",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(5)
	//	DeviceWrite(lbneReg.readout_window[6],		event_packet_length	, deviceNum);	//	X"158",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(6)
	//	DeviceWrite(lbneReg.readout_window[7],		event_packet_length	, deviceNum);	//	X"15C",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(7)
	//	DeviceWrite(lbneReg.readout_window[8],		event_packet_length	, deviceNum);	//	X"160",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(8)
	//	DeviceWrite(lbneReg.readout_window[9],		event_packet_length	, deviceNum);	//	X"164",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(9)
	//	DeviceWrite(lbneReg.readout_window[10],		event_packet_length	, deviceNum);	//	X"168",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(10)
	//	DeviceWrite(lbneReg.readout_window[11],		event_packet_length	, deviceNum);	//	X"16C",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(11)

		for (i = 0; i < MAX_CHANNELS; i++) data[i] = p_window;
		DeviceArrayWrite(lbneReg.p_window[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.p_window[0],			p_window			, deviceNum);	//	X"180",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(0)
	//	DeviceWrite(lbneReg.p_window[1],			p_window			, deviceNum);	//	X"184",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(1)
	//	DeviceWrite(lbneReg.p_window[2],			p_window			, deviceNum);	//	X"188",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(2)
	//	DeviceWrite(lbneReg.p_window[3],			p_window			, deviceNum);	//	X"18C",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(3)
	//	DeviceWrite(lbneReg.p_window[4],			p_window			, deviceNum);	//	X"190",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(4)
	//	DeviceWrite(lbneReg.p_window[5],			p_window			, deviceNum);	//	X"194",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(5)
	//	DeviceWrite(lbneReg.p_window[6],			p_window			, deviceNum);	//	X"198",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(6)
	//	DeviceWrite(lbneReg.p_window[7],			p_window			, deviceNum);	//	X"19C",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(7)
	//	DeviceWrite(lbneReg.p_window[8],			p_window			, deviceNum);	//	X"1A0",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(8)
	//	DeviceWrite(lbneReg.p_window[9],			p_window			, deviceNum);	//	X"1A4",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(9)
	//	DeviceWrite(lbneReg.p_window[10],			p_window			, deviceNum);	//	X"1A8",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(10)
	//	DeviceWrite(lbneReg.p_window[11],			p_window			, deviceNum);	//	X"1AC",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(11)

		for (i = 0; i < MAX_CHANNELS; i++) data[i] = i2_window;
		DeviceArrayWrite(lbneReg.i2_window[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.i2_window[0],			i2_window			, deviceNum);	//	X"1C0",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(0)
	//	DeviceWrite(lbneReg.i2_window[1],			i2_window			, deviceNum);	//	X"1C4",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(1)
	//	DeviceWrite(lbneReg.i2_window[2],			i2_window			, deviceNum);	//	X"1C8",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(2)
	//	DeviceWrite(lbneReg.i2_window[3],			i2_window			, deviceNum);	//	X"1CC",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(3)
	//	DeviceWrite(lbneReg.i2_window[4],			i2_window			, deviceNum);	//	X"1D0",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(4)
	//	DeviceWrite(lbneReg.i2_window[5],			i2_window			, deviceNum);	//	X"1D4",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(5)
	//	DeviceWrite(lbneReg.i2_window[6],			i2_window			, deviceNum);	//	X"1D8",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(6)
	//	DeviceWrite(lbneReg.i2_window[7],			i2_window			, deviceNum);	//	X"1DC",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(7)
	//	DeviceWrite(lbneReg.i2_window[8],			i2_window			, deviceNum);	//	X"1E0",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(8)
	//	DeviceWrite(lbneReg.i2_window[9],			i2_window			, deviceNum);	//	X"1E4",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(9)
	//	DeviceWrite(lbneReg.i2_window[10],			i2_window			, deviceNum);	//	X"1E8",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(10)
	//	DeviceWrite(lbneReg.i2_window[11],			i2_window			, deviceNum);	//	X"1EC",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(11)

		for (i = 0; i < MAX_CHANNELS; i++) data[i] = m1_window;
		DeviceArrayWrite(lbneReg.m1_window[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.m1_window[0],			m1_window			, deviceNum);	//	X"200",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(0)
	//	DeviceWrite(lbneReg.m1_window[1],			m1_window			, deviceNum);	//	X"204",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(1)
	//	DeviceWrite(lbneReg.m1_window[2],			m1_window			, deviceNum);	//	X"208",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(2)
	//	DeviceWrite(lbneReg.m1_window[3],			m1_window			, deviceNum);	//	X"20C",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(3)
	//	DeviceWrite(lbneReg.m1_window[4],			m1_window			, deviceNum);	//	X"210",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(4)
	//	DeviceWrite(lbneReg.m1_window[5],			m1_window			, deviceNum);	//	X"214",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(5)
	//	DeviceWrite(lbneReg.m1_window[6],			m1_window			, deviceNum);	//	X"218",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(6)
	//	DeviceWrite(lbneReg.m1_window[7],			m1_window			, deviceNum);	//	X"21C",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(7)
	//	DeviceWrite(lbneReg.m1_window[8],			m1_window			, deviceNum);	//	X"220",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(8)
	//	DeviceWrite(lbneReg.m1_window[9],			m1_window			, deviceNum);	//	X"224",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(9)
	//	DeviceWrite(lbneReg.m1_window[10],			m1_window			, deviceNum);	//	X"228",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(10)
	//	DeviceWrite(lbneReg.m1_window[11],			m1_window			, deviceNum);	//	X"22C",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(11)

		for (i = 0; i < MAX_CHANNELS; i++) data[i] = m2_window;
		DeviceArrayWrite(lbneReg.m2_window[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.m2_window[0],			m2_window			, deviceNum);	//	X"240",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(0)
	//	DeviceWrite(lbneReg.m2_window[1],			m2_window			, deviceNum);	//	X"244",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(1)
	//	DeviceWrite(lbneReg.m2_window[2],			m2_window			, deviceNum);	//	X"248",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(2)
	//	DeviceWrite(lbneReg.m2_window[3],			m2_window			, deviceNum);	//	X"24C",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(3)
	//	DeviceWrite(lbneReg.m2_window[4],			m2_window			, deviceNum);	//	X"250",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(4)
	//	DeviceWrite(lbneReg.m2_window[5],			m2_window			, deviceNum);	//	X"254",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(5)
	//	DeviceWrite(lbneReg.m2_window[6],			m2_window			, deviceNum);	//	X"258",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(6)
	//	DeviceWrite(lbneReg.m2_window[7],			m2_window			, deviceNum);	//	X"25C",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(7)
	//	DeviceWrite(lbneReg.m2_window[8],			m2_window			, deviceNum);	//	X"260",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(8)
	//	DeviceWrite(lbneReg.m2_window[9],			m2_window			, deviceNum);	//	X"264",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(9)
	//	DeviceWrite(lbneReg.m2_window[10],			m2_window			, deviceNum);	//	X"268",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(10)
	//	DeviceWrite(lbneReg.m2_window[11],			m2_window			, deviceNum);	//	X"26C",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(11)

		for (i = 0; i < MAX_CHANNELS; i++) data[i] = d_window;
		DeviceArrayWrite(lbneReg.d_window[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.d_window[0],			d_window			, deviceNum);	//	X"280",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(0)
	//	DeviceWrite(lbneReg.d_window[1],			d_window			, deviceNum);	//	X"284",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(1)
	//	DeviceWrite(lbneReg.d_window[2],			d_window			, deviceNum);	//	X"288",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(2)
	//	DeviceWrite(lbneReg.d_window[3],			d_window			, deviceNum);	//	X"28C",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(3)
	//	DeviceWrite(lbneReg.d_window[4],			d_window			, deviceNum);	//	X"290",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(4)
	//	DeviceWrite(lbneReg.d_window[5],			d_window			, deviceNum);	//	X"294",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(5)
	//	DeviceWrite(lbneReg.d_window[6],			d_window			, deviceNum);	//	X"298",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(6)
	//	DeviceWrite(lbneReg.d_window[7],			d_window			, deviceNum);	//	X"29C",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(7)
	//	DeviceWrite(lbneReg.d_window[8],			d_window			, deviceNum);	//	X"2A0",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(8)
	//	DeviceWrite(lbneReg.d_window[9],			d_window			, deviceNum);	//	X"2A4",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(9)
	//	DeviceWrite(lbneReg.d_window[10],			d_window			, deviceNum);	//	X"2A8",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(10)
	//	DeviceWrite(lbneReg.d_window[11],			d_window			, deviceNum);	//	X"2AC",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(11)

		for (i = 0; i < MAX_CHANNELS; i++) data[i] = i1_window;
		DeviceArrayWrite(lbneReg.i1_window[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.i1_window[0],			i1_window			, deviceNum);	//	X"2C0",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(0)
	//	DeviceWrite(lbneReg.i1_window[1],			i1_window			, deviceNum);	//	X"2C4",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(1)
	//	DeviceWrite(lbneReg.i1_window[2],			i1_window			, deviceNum);	//	X"2C8",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(2)
	//	DeviceWrite(lbneReg.i1_window[3],			i1_window			, deviceNum);	//	X"2CC",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(3)
	//	DeviceWrite(lbneReg.i1_window[4],			i1_window			, deviceNum);	//	X"2D0",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(4)
	//	DeviceWrite(lbneReg.i1_window[5],			i1_window			, deviceNum);	//	X"2D4",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(5)
	//	DeviceWrite(lbneReg.i1_window[6],			i1_window			, deviceNum);	//	X"2D8",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(6)
	//	DeviceWrite(lbneReg.i1_window[7],			i1_window			, deviceNum);	//	X"2DC",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(7)
	//	DeviceWrite(lbneReg.i1_window[8],			i1_window			, deviceNum);	//	X"2E0",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(8)
	//	DeviceWrite(lbneReg.i1_window[9],			i1_window			, deviceNum);	//	X"2E4",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(9)
	//	DeviceWrite(lbneReg.i1_window[10],			i1_window			, deviceNum);	//	X"2E8",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(10)
	//	DeviceWrite(lbneReg.i1_window[11],			i1_window			, deviceNum);	//	X"2EC",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(11)

		for (i = 0; i < MAX_CHANNELS; i++) data[i] = disc_width;
		DeviceArrayWrite(lbneReg.disc_width[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.disc_width[0],			disc_width			, deviceNum);	//	X"300",		X"00000000",	X"0000003F",	X"0000003F",	reg_disc_width(0)
	//	DeviceWrite(lbneReg.disc_width[1],			disc_width			, deviceNum);	//	X"304",		X"00000000",	X"0000003F",	X"0000003F",	reg_disc_width(1)
	//	DeviceWrite(lbneReg.disc_width[2],			disc_width			, deviceNum);	//	X"308",		X"00000000",	X"0000003F",	X"0000003F",	reg_disc_width(2)
	//	DeviceWrite(lbneReg.disc_width[3],			disc_width			, deviceNum);	//	X"30C",		X"00000000",	X"0000003F",	X"0000003F",	reg_disc_width(3)
	//	DeviceWrite(lbneReg.disc_width[4],			disc_width			, deviceNum);	//	X"310",		X"00000000",	X"0000003F",	X"0000003F",	reg_disc_width(4)
	//	DeviceWrite(lbneReg.disc_width[5],			disc_width			, deviceNum);	//	X"314",		X"00000000",	X"0000003F",	X"0000003F",	reg_disc_width(5)
	//	DeviceWrite(lbneReg.disc_width[6],			disc_width			, deviceNum);	//	X"318",		X"00000000",	X"0000003F",	X"0000003F",	reg_disc_width(6)
	//	DeviceWrite(lbneReg.disc_width[7],			disc_width			, deviceNum);	//	X"31C",		X"00000000",	X"0000003F",	X"0000003F",	reg_disc_width(7)
	//	DeviceWrite(lbneReg.disc_width[8],			disc_width			, deviceNum);	//	X"320",		X"00000000",	X"0000003F",	X"0000003F",	reg_disc_width(8)
	//	DeviceWrite(lbneReg.disc_width[9],			disc_width			, deviceNum);	//	X"324",		X"00000000",	X"0000003F",	X"0000003F",	reg_disc_width(9)
	//	DeviceWrite(lbneReg.disc_width[10],			disc_width			, deviceNum);	//	X"328",		X"00000000",	X"0000003F",	X"0000003F",	reg_disc_width(10)
	//	DeviceWrite(lbneReg.disc_width[11],			disc_width			, deviceNum);	//	X"32C",		X"00000000",	X"0000003F",	X"0000003F",	reg_disc_width(11)

		for (i = 0; i < MAX_CHANNELS; i++) data[i] = baseline_start;
		DeviceArrayWrite(lbneReg.baseline_start[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.baseline_start[0],		baseline_start		, deviceNum);	//	X"340",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(0)
	//	DeviceWrite(lbneReg.baseline_start[1],		baseline_start		, deviceNum);	//	X"344",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(1)
	//	DeviceWrite(lbneReg.baseline_start[2],		baseline_start		, deviceNum);	//	X"348",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(2)
	//	DeviceWrite(lbneReg.baseline_start[3],		baseline_start		, deviceNum);	//	X"34C",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(3)
	//	DeviceWrite(lbneReg.baseline_start[4],		baseline_start		, deviceNum);	//	X"350",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(4)
	//	DeviceWrite(lbneReg.baseline_start[5],		baseline_start		, deviceNum);	//	X"354",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(5)
	//	DeviceWrite(lbneReg.baseline_start[6],		baseline_start		, deviceNum);	//	X"358",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(6)
	//	DeviceWrite(lbneReg.baseline_start[7],		baseline_start		, deviceNum);	//	X"35C",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(7)
	//	DeviceWrite(lbneReg.baseline_start[8],		baseline_start		, deviceNum);	//	X"360",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(8)
	//	DeviceWrite(lbneReg.baseline_start[9],		baseline_start		, deviceNum);	//	X"364",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(9)
	//	DeviceWrite(lbneReg.baseline_start[10],		baseline_start		, deviceNum);	//	X"368",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(10)
	//	DeviceWrite(lbneReg.baseline_start[11],		baseline_start		, deviceNum);	//	X"36C",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(11)

		DeviceWrite(lbneReg.trigger_input_delay,	0x00000001			, deviceNum);	//	X"400",		X"00000020",	X"0000FFFF",	X"0000FFFF",	reg_trigger_input_delay
		DeviceWrite(lbneReg.gpio_output_width,		0x00001000			, deviceNum);	//	X"404",		X"0000000F",	X"0000FFFF",	X"0000FFFF",	reg_gpio_output_width
		DeviceWrite(lbneReg.front_panel_config, 	0x00001111			, deviceNum);	//	X"408",		X"00001101",	X"00773333",	X"00773333",	reg_front_panel_config
	//	DeviceWrite(lbneReg.channel_pulsed_control, Write_Only			, deviceNum);	//	X"40C",		X"00000000",	X"00000000",	X"FFFFFFFF",	reg_channel_pulsed_control
		DeviceWrite(lbneReg.dsp_led_config,			0x00000000			, deviceNum);	//	X"410",		X"00000000",	X"FFFFFFFF",	X"00000001",	reg_dsp_led_config
		DeviceWrite(lbneReg.dsp_led_input, 			0x00000000			, deviceNum);	//	X"414",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_dsp_led_input
		DeviceWrite(lbneReg.baseline_delay,			baseline_delay		, deviceNum);	//	X"418",		X"00000019",	X"000000FF",	X"000000FF",	reg_baseline_delay
		DeviceWrite(lbneReg.diag_channel_input,		0x00000000			, deviceNum);	//	X"41C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_diag_channel_input
	//	DeviceWrite(lbneReg.dps_pulsed_control, 	Write_Only			, deviceNum);	//	X"420",		X"00000000",	X"00000000",	X"FFFFFFFF",	reg_dsp_pulsed_control
	//	DeviceWrite(lbneReg.event_data_control, 	In DeviceStart/Stop	, deviceNum);	//	X"424",		X"00000001",	X"00020001",	X"00020001",	reg_event_data_control
	//	DeviceWrite(lbneReg.adc_config, 			Do Not Override FW	, deviceNum);	//	X"428",		X"00010000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_adc_config
	//	DeviceWrite(lbneReg.adc_config_load, 		Write_Only			, deviceNum);	//	X"42C",		X"00000000",	X"00000000",	X"00000001",	reg_adc_config_load
		DeviceWrite(lbneReg.qi_config,				0x0FFF1F00			, deviceNum);	//	X"430",		X"0FFF1F00",	X"0FFF1F11",	X"0FFF1F11",	reg_qi_config
		DeviceWrite(lbneReg.qi_delay,				0x00000000			, deviceNum);	//	X"434",		X"00000000",	X"0000007F",	X"0000007F",	reg_qi_delay
		DeviceWrite(lbneReg.qi_pulse_width,			0x00000000			, deviceNum);	//	X"438",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_qi_pulse_width
	//	DeviceWrite(lbneReg.qi_pulsed,				Write_Only			, deviceNum);	//	X"43C",		X"00000000",	X"00000000",	X"00030001",	reg_qi_pulsed
		DeviceWrite(lbneReg.external_gate_width,	0x00008000			, deviceNum);	//	X"440",		X"00008000",	X"0000FFFF",	X"0000FFFF",	reg_external_gate_width
	//	DeviceWrite(lbneReg.lat_timestamp_lsb, 		Read_Only			, deviceNum);	//	X"484",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_lat_timestamp (lsb)
	//	DeviceWrite(lbneReg.lat_timestamp_msb, 		Read_Only			, deviceNum);	//	X"488",		X"00000000",	X"0000FFFF",	X"00000000",	reg_lat_timestamp (msb)
	//	DeviceWrite(lbneReg.live_timestamp_lsb, 	Read_Only			, deviceNum);	//	X"48C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_live_timestamp (lsb)
	//	DeviceWrite(lbneReg.live_timestamp_msb,		Read_Only			, deviceNum);	//	X"490",		X"00000000",	X"0000FFFF",	X"00000000",	reg_live_timestamp (msb)
	//	DeviceWrite(lbneReg.sync_period,		 	Read_Only			, deviceNum);	//	X"494",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_last_sync_reset_count
	//	DeviceWrite(lbneReg.sync_delay,				Read_Only			, deviceNum);	//	X"498",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_external_timestamp (lsb)
	//	DeviceWrite(lbneReg.sync_count,				Read_Only			, deviceNum);	//	X"49C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_external_timestamp (msb)

	//	DeviceWrite(lbneReg.master_logic_control, 	In DeviceStart/Stop	, deviceNum);	//	X"500",		X"00000000",	X"FFFFFFFF",	X"00000073",	reg_master_logic_control
	//	DeviceWrite(lbneReg.overflow_status, 		Read_Only			, deviceNum);	//	X"508",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_overflow_status
	//	DeviceWrite(lbneReg.phase_value, 			Read_Only			, deviceNum);	//	X"50C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_phase_value
	//	DeviceWrite(lbneReg.link_tx_status, 		Read_Only			, deviceNum);	//	X"510",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_link_tx_status
		DeviceWrite(lbneReg.dsp_clock_control,		0x00000000			, deviceNum);	//	X"520",		X"00000000",	X"00000713",	X"00000713",	reg_dsp_clock_control		
    //	DeviceWrite(lbneReg.dsp_clock_phase_control,Write_Only			, deviceNum);	//	X"524",		X"00000000",	X"00000000",	X"00000007",	reg_dsp_clock_phase_control

	//	DeviceWrite(lbneReg.code_revision, 			Read_Only			, deviceNum);	//	X"600",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_code_revision
	//	DeviceWrite(lbneReg.code_date, 				Read_Only			, deviceNum);	//	X"604",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_code_date

	//	for (i = 0; i < MAX_CHANNELS; i++) data[i] = Read_Only;
	//	DeviceArrayWrite(lbneReg.dropped_event_count[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.dropped_event_count[0], Read_Only			, deviceNum);	//	X"700",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(0)
	//	DeviceWrite(lbneReg.dropped_event_count[1], Read_Only			, deviceNum);	//	X"704",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(1)
	//	DeviceWrite(lbneReg.dropped_event_count[2], Read_Only			, deviceNum);	//	X"708",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(2)
	//	DeviceWrite(lbneReg.dropped_event_count[3], Read_Only			, deviceNum);	//	X"70C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(3)
	//	DeviceWrite(lbneReg.dropped_event_count[4], Read_Only			, deviceNum);	//	X"710",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(4)
	//	DeviceWrite(lbneReg.dropped_event_count[5], Read_Only			, deviceNum);	//	X"714",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(5)
	//	DeviceWrite(lbneReg.dropped_event_count[6], Read_Only			, deviceNum);	//	X"718",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(6)
	//	DeviceWrite(lbneReg.dropped_event_count[7], Read_Only			, deviceNum);	//	X"71C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(7)
	//	DeviceWrite(lbneReg.dropped_event_count[8], Read_Only			, deviceNum);	//	X"720",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(8)
	//	DeviceWrite(lbneReg.dropped_event_count[9], Read_Only			, deviceNum);	//	X"724",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(9)
	//	DeviceWrite(lbneReg.dropped_event_count[10],Read_Only			, deviceNum);	//	X"728",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(10)
	//	DeviceWrite(lbneReg.dropped_event_count[11],Read_Only			, deviceNum);	//	X"72C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(11)

	//	for (i = 0; i < MAX_CHANNELS; i++) data[i] = Read_Only;
	//	DeviceArrayWrite(lbneReg.accepted_event_count[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.accepted_event_count[0],Read_Only 			, deviceNum);	//	X"740",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(0)
	//	DeviceWrite(lbneReg.accepted_event_count[1],Read_Only 			, deviceNum);	//	X"744",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(1)
	//	DeviceWrite(lbneReg.accepted_event_count[2],Read_Only 			, deviceNum);	//	X"748",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(2)
	//	DeviceWrite(lbneReg.accepted_event_count[3],Read_Only 			, deviceNum);	//	X"74C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(3)
	//	DeviceWrite(lbneReg.accepted_event_count[4],Read_Only 			, deviceNum);	//	X"750",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(4)
	//	DeviceWrite(lbneReg.accepted_event_count[5],Read_Only 			, deviceNum);	//	X"754",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(5)
	//	DeviceWrite(lbneReg.accepted_event_count[6],Read_Only 			, deviceNum);	//	X"758",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(6)
	//	DeviceWrite(lbneReg.accepted_event_count[7],Read_Only 			, deviceNum);	//	X"75C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(7)
	//	DeviceWrite(lbneReg.accepted_event_count[8],Read_Only 			, deviceNum);	//	X"760",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(8)
	//	DeviceWrite(lbneReg.accepted_event_count[9],Read_Only 			, deviceNum);	//	X"764",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(9)
	//	DeviceWrite(lbneReg.accepted_event_count[10],Read_Only 			, deviceNum);	//	X"768",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(10)
	//	DeviceWrite(lbneReg.accepted_event_count[11],Read_Only 			, deviceNum);	//	X"76C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(11)

	//	for (i = 0; i < MAX_CHANNELS; i++) data[i] = Read_Only;
	//	DeviceArrayWrite(lbneReg.ahit_count[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.ahit_count[0], 			Read_Only			, deviceNum);	//	X"780",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(0)
	//	DeviceWrite(lbneReg.ahit_count[1], 			Read_Only			, deviceNum);	//	X"784",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(1)
	//	DeviceWrite(lbneReg.ahit_count[2], 			Read_Only			, deviceNum);	//	X"788",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(2)
	//	DeviceWrite(lbneReg.ahit_count[3], 			Read_Only			, deviceNum);	//	X"78C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(3)
	//	DeviceWrite(lbneReg.ahit_count[4], 			Read_Only			, deviceNum);	//	X"790",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(4)
	//	DeviceWrite(lbneReg.ahit_count[5], 			Read_Only			, deviceNum);	//	X"794",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(5)
	//	DeviceWrite(lbneReg.ahit_count[6], 			Read_Only			, deviceNum);	//	X"798",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(6)
	//	DeviceWrite(lbneReg.ahit_count[7], 			Read_Only			, deviceNum);	//	X"79C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(7)
	//	DeviceWrite(lbneReg.ahit_count[8], 			Read_Only			, deviceNum);	//	X"7A0",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(8)
	//	DeviceWrite(lbneReg.ahit_count[9], 			Read_Only			, deviceNum);	//	X"7A4",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(9)
	//	DeviceWrite(lbneReg.ahit_count[10], 		Read_Only			, deviceNum);	//	X"7A8",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(10)
	//	DeviceWrite(lbneReg.ahit_count[11], 		Read_Only			, deviceNum);	//	X"7AC",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(11)

	//	for (i = 0; i < MAX_CHANNELS; i++) data[i] = Read_Only;
	//	DeviceArrayWrite(lbneReg.disc_count[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.disc_count[0], 			Read_Only			, deviceNum);	//	X"7C0",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(0)
	//	DeviceWrite(lbneReg.disc_count[1], 			Read_Only			, deviceNum);	//	X"7C4",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(1)
	//	DeviceWrite(lbneReg.disc_count[2], 			Read_Only			, deviceNum);	//	X"7C8",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(2)
	//	DeviceWrite(lbneReg.disc_count[3], 			Read_Only			, deviceNum);	//	X"7CC",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(3)
	//	DeviceWrite(lbneReg.disc_count[4], 			Read_Only			, deviceNum);	//	X"7D0",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(4)
	//	DeviceWrite(lbneReg.disc_count[5], 			Read_Only			, deviceNum);	//	X"7D4",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(5)
	//	DeviceWrite(lbneReg.disc_count[6], 			Read_Only			, deviceNum);	//	X"7D8",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(6)
	//	DeviceWrite(lbneReg.disc_count[7], 			Read_Only			, deviceNum);	//	X"7DC",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(7)
	//	DeviceWrite(lbneReg.disc_count[8], 			Read_Only			, deviceNum);	//	X"7E0",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(8)
	//	DeviceWrite(lbneReg.disc_count[9], 			Read_Only			, deviceNum);	//	X"7E4",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(9)
	//	DeviceWrite(lbneReg.disc_count[10], 		Read_Only			, deviceNum);	//	X"7E8",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(10)
	//	DeviceWrite(lbneReg.disc_count[11], 		Read_Only			, deviceNum);	//	X"7EC",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(11)

	//	for (i = 0; i < MAX_CHANNELS; i++) data[i] = Read_Only;
	//	DeviceArrayWrite(lbneReg.idelay_count[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.idelay_count[0], 		Read_Only			, deviceNum);	//	X"800",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(0)
	//	DeviceWrite(lbneReg.idelay_count[1], 		Read_Only			, deviceNum);	//	X"804",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(1)
	//	DeviceWrite(lbneReg.idelay_count[2], 		Read_Only			, deviceNum);	//	X"808",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(2)
	//	DeviceWrite(lbneReg.idelay_count[3], 		Read_Only			, deviceNum);	//	X"80C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(3)
	//	DeviceWrite(lbneReg.idelay_count[4], 		Read_Only			, deviceNum);	//	X"810",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(4)
	//	DeviceWrite(lbneReg.idelay_count[5], 		Read_Only			, deviceNum);	//	X"814",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(5)
	//	DeviceWrite(lbneReg.idelay_count[6], 		Read_Only			, deviceNum);	//	X"818",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(6)
	//	DeviceWrite(lbneReg.idelay_count[7], 		Read_Only			, deviceNum);	//	X"81C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(7)
	//	DeviceWrite(lbneReg.idelay_count[8], 		Read_Only			, deviceNum);	//	X"820",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(8)
	//	DeviceWrite(lbneReg.idelay_count[9], 		Read_Only			, deviceNum);	//	X"824",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(9)
	//	DeviceWrite(lbneReg.idelay_count[10], 		Read_Only			, deviceNum);	//	X"828",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(10)
	//	DeviceWrite(lbneReg.idelay_count[11], 		Read_Only			, deviceNum);	//	X"82C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(11)

	//	for (i = 0; i < MAX_CHANNELS; i++) data[i] = Read_Only;
	//	DeviceArrayWrite(lbneReg.adc_data_monitor[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.adc_data_monitor[0], 	Read_Only			, deviceNum);	//	X"840",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(0)
	//	DeviceWrite(lbneReg.adc_data_monitor[1], 	Read_Only			, deviceNum);	//	X"844",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(1)
	//	DeviceWrite(lbneReg.adc_data_monitor[2], 	Read_Only			, deviceNum);	//	X"848",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(2)
	//	DeviceWrite(lbneReg.adc_data_monitor[3], 	Read_Only			, deviceNum);	//	X"84C",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(3)
	//	DeviceWrite(lbneReg.adc_data_monitor[4], 	Read_Only			, deviceNum);	//	X"850",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(4)
	//	DeviceWrite(lbneReg.adc_data_monitor[5], 	Read_Only			, deviceNum);	//	X"854",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(5)
	//	DeviceWrite(lbneReg.adc_data_monitor[6], 	Read_Only			, deviceNum);	//	X"858",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(6)
	//	DeviceWrite(lbneReg.adc_data_monitor[7], 	Read_Only			, deviceNum);	//	X"85C",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(7)
	//	DeviceWrite(lbneReg.adc_data_monitor[8], 	Read_Only			, deviceNum);	//	X"860",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(8)
	//	DeviceWrite(lbneReg.adc_data_monitor[9], 	Read_Only			, deviceNum);	//	X"864",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(9)
	//	DeviceWrite(lbneReg.adc_data_monitor[10], 	Read_Only			, deviceNum);	//	X"868",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(10)
	//	DeviceWrite(lbneReg.adc_data_monitor[11], 	Read_Only			, deviceNum);	//	X"86C",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(11)

	//	for (i = 0; i < MAX_CHANNELS; i++) data[i] = Read_Only;
	//	DeviceArrayWrite(lbneReg.adc_status[0], MAX_CHANNELS, data, deviceNum);
	//	DeviceWrite(lbneReg.adc_status[0], 			Read_Only			, deviceNum);	//	X"880",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(0)
	//	DeviceWrite(lbneReg.adc_status[1], 			Read_Only			, deviceNum);	//	X"884",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(1)
	//	DeviceWrite(lbneReg.adc_status[2], 			Read_Only			, deviceNum);	//	X"888",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(2)
	//	DeviceWrite(lbneReg.adc_status[3], 			Read_Only			, deviceNum);	//	X"88C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(3)
	//	DeviceWrite(lbneReg.adc_status[4], 			Read_Only			, deviceNum);	//	X"890",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(4)
	//	DeviceWrite(lbneReg.adc_status[5], 			Read_Only			, deviceNum);	//	X"894",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(5)
	//	DeviceWrite(lbneReg.adc_status[6], 			Read_Only			, deviceNum);	//	X"898",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(6)
	//	DeviceWrite(lbneReg.adc_status[7], 			Read_Only			, deviceNum);	//	X"89C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(7)
	//	DeviceWrite(lbneReg.adc_status[8], 			Read_Only			, deviceNum);	//	X"8A0",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(8)
	//	DeviceWrite(lbneReg.adc_status[9], 			Read_Only			, deviceNum);	//	X"8A4",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(9)
	//	DeviceWrite(lbneReg.adc_status[10], 		Read_Only			, deviceNum);	//	X"8A8",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(10)
	//	DeviceWrite(lbneReg.adc_status[11], 		Read_Only			, deviceNum);	//	X"8AC",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(11)

	// Load the window settings - This MUST be the last operation
	DeviceWrite(lbneReg.channel_pulsed_control, 0x1, deviceNum);

	// Check timestamp at end of DeviceSetup()
	DeviceRead(lbneReg.live_timestamp_lsb, &endTime, deviceNum);

	// Calculate time to complete DeviceSetup()
	// Using 100MHz clock, calculate time in us
	elapsedTime = ((endTime - startTime) * 10.0) / 1000.0;

	return 0;
}

int DeviceStart (int deviceNum)
{
	// This script enables all logic and FIFOs and starts data acquisition in the device
	// Operations MUST be performed in this order

	// Registers in the Artix FPGA (DSP)									//	Default Value	Read Mask		Write Mask		Code Name
	// Enable the links and flags							
        DeviceWrite(lbneReg.event_data_control,					0x00000000	, deviceNum);	//	X"00000001",	X"00020001",	X"00020001",	reg_event_data_control
	// Release the FIFO reset						
	DeviceWrite(lbneReg.fifo_control,						0x00000000	, deviceNum);	//	X"00000000",	X"0FFFFFFF",	X"08000000",	reg_fifo_control

	// Registers in the Zynq FPGA (Comm)
	// Reset the links and flags
	DeviceWrite(lbneReg.eventDataControl,					0x00000000	, deviceNum);	//	X"0000001F",	X"FFFFFFFF",	X"0013001F",	reg_event_data_control

	// Registers in the Artix FPGA (DSP)
	// Release master logic reset & enable active channels
	DeviceSet(lbneReg.master_logic_control,					0x00000001	, deviceNum);	//	X"00000000",	X"FFFFFFFF",	X"00000073",	reg_master_logic_status

	return 0;
}

int DeviceStopReset (int deviceNum)
{
	int error = 0;

	// This script resets all logic and FIFOs and flushes the software receive buffer
	// Operations MUST be performed in this order

	// Registers in the Zynq FPGA (Comm)									//	Default Value	Read Mask		Write Mask		Code Name
	// Reset the links and flags
	DeviceWrite(lbneReg.eventDataControl,					0x0013001F	, deviceNum);	//	X"0000001F",	X"FFFFFFFF",	X"0013001F",	reg_event_data_control

	// Registers in the Artix FPGA (DSP)
	// Clear the master logic enable
	DeviceClear(lbneReg.master_logic_control,				0x00000001	, deviceNum);	//	X"00000000",	X"FFFFFFFF",	X"00000073",	reg_master_logic_status
	// Clear the FIFOs
	DeviceWrite(lbneReg.fifo_control,						0x08000000	, deviceNum);	//	X"00000000",	X"0FFFFFFF",	X"08000000",	reg_fifo_control
	// Clear the DDR
	DeviceWrite(lbneReg.PurgeDDR,							0x00000001	, deviceNum);
	// Reset the links and flags				
	DeviceWrite(lbneReg.event_data_control,					0x00020001	, deviceNum);	//	X"00000001",	X"00020001",	X"00020001",	reg_event_data_control

	// Software Level
	// Flush RX buffer in USB chip and driver
	error = DevicePurgeData(deviceNum);
	
	return error;
}

//==============================================================================
// Log Functions
//==============================================================================

#ifdef ANL
void LogComment (const char format[], ...)
{
	va_list args;
	
	if (logging) {			 
		va_start(args, format);
		vfprintf(logFile, format, args);
		va_end(args);
	}
}

void LogData (void* data, uint dataBytes)
{
	uint i = 0;
	uint dataUints	= dataBytes / 4;
	uint dataShorts = dataBytes / 2;
	
	if (logging) {
		if (dataBytes != (dataUints * 4)) {
			sprintf(logEntry, "LOG: %d BYTES\n", dataBytes);
			fputs(logEntry, logFile);
		}
		
		for(i = 0; i < dataShorts; i++){
			if(dataSynced == 1) {
				sprintf(logEntry, "%04X\n", ((ushort*)data)[i]);
			} else {
				sprintf(logEntry, "%04X _BAD\n", ((ushort*)data)[i]);
			}
			fputs(logEntry, logFile);
		}
		
		if (dataBytes != (dataShorts * 2)) {
			sprintf(logEntry, "%02X _EXTRA\n", ((uchar*)data)[dataBytes - 1]);
			fputs(logEntry, logFile);
		}
	}
}

void LogStart (void)
{
#if defined(ENABLE_LOG)
	time(&logMachineTime);
	logTime = localtime(&logMachineTime);
	strftime(logFilename, MAX_PATHNAME_LEN, "../Logs/Log_%Y_%m_%d_%H_%M_%S.log", logTime);
	logFile = fopen(logFilename,"a");
	logEvent = 0;
	logging = 1;
#endif
}

void LogStop (void)
{
	if (logging) {
		fclose(logFile);
		logging = 0;
	}
}
#endif
