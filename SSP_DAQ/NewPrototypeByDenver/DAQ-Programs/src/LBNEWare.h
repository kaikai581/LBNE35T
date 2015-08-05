//==============================================================================
//
// Title:		LBNEWare.h
// Description:	Diagnostic Software for the LBNE Digitizer
//				This software will provide access to diagnostic registers and
//				memory within the module's ARM processor, Zynq FPGA, and Artix
//				FPGA.  The host PC runs a custom control protocol while the
//				module uses a data push architecture for event data.  Initially,
//				the software will use the FTDI FT2232H_mini_module which has a
//				USB serial port for the control path and another USB data device
//				for the data path.  In the future, these communication paths
//				can be redirected to a gigabit Ethernet link.
// Author:		Andrew Kreps
//
//==============================================================================

#ifndef __LBNE_WARE_H__
#define __LBNE_WARE_H__

//==============================================================================
// Constants
//==============================================================================

#define MAX_BINS			1024	// Maximum bins in histogram
#define MAX_CHANNELS		12		// Maximum number of channels in digitizer
#define MAX_DISPLAY_DATA	256		// Maximum size of LabWindows table
#define MAX_EVENT_DATA		2046	// Maximum size of waveform in an event
#define MAX_EVENTS			50000	// Maximum events to store from digitizer
#define MAX_PATHNAME_LEN	260		// Maximum length of path including null byte
#define MAX_TIMING_MARKS	32		// Maximum number of timing marks to track per event

#define ENABLE_LOG					// If defined, writes every event data word to file.
#define ERROR_MESSAGE				// If defined, pops up error messages during I/O
//#define ANL						// If defined, includes code needed for LabWindows
//#define ANL_TCP
//#define ANL_UDP

#define COMM_CONFIG_ROM_ADDRESS		0x20000000
#define DSP_A_CONFIG_ROM_ADDRESS	0x20010000
#define DSP_B_CONFIG_ROM_ADDRESS	0x20020000
#define DSP_C_CONFIG_ROM_ADDRESS	0x20030000
#define MODULE_INFO_ROM_ADDRESS		0x20100000

//==============================================================================
// Enumerated Constants
//==============================================================================

enum errorConstants {
	errorNoError			= 0,

	// Device Communication Errors
	errorCommConnect		= 1,
	errorCommDisconnect		= 2,
	errorCommDiscover		= 3,
	errorCommReceive		= 4,
	errorCommSend			= 5,
	errorCommReceiveZero	= 6,
	errorCommReceiveTimeout	= 7,
	errorCommSendZero		= 8,
	errorCommSendTimeout	= 9,
	errorCommType			= 10,
	errorCommPurge			= 11,
	errorCommQueue			= 12,
	
	// Device Data Errors
	errorDataConnect		= 101,
	errorDataLength			= 102,
	errorDataPurge			= 103,
	errorDataQueue			= 104,
	errorDataReceive		= 105,
	errorDataTimeout		= 106,
	errorDataDisconnect		= 107,
	
	// LBNE Errors
	errorEventTooLarge		= 201,
	errorEventTooSmall		= 202,
	errorEventTooMany		= 203,
	errorEventHeader		= 204
};

//==============================================================================
// Types
//==============================================================================

typedef unsigned char		uchar;	// 1 byte / 8 bit
typedef unsigned short		ushort;	// 2 byte / 16 bit
typedef unsigned int		uint;	// 4 byte / 32 bit
typedef unsigned long int	uint64;	// 8 byte / 64 bit

typedef struct {
	uchar	pileup			: 1;
	uchar	polarity		: 1;
	uchar	offsetReadout	: 1;
	uchar	cfdValid		: 1;
	uchar	reserved		: 4;
} EventFlags;

typedef union {
	uchar		flags;
	EventFlags	flag;
} EventStatus;

typedef struct _Event_Header {	// NOTE: Group fields are listed from MSB to LSB
	uint	header;				// 0xAAAAAAAA
	ushort	length;				// Packet Length in uints (including header)
	ushort	group1;				// Trigger Type, Status Flags, Header Type
	ushort	triggerID;			// Trigger ID
	ushort	group2;				// Module ID, Channel ID
	ushort	timestamp[4];		// External Timestamp
								// Words 0-1 = Clocks since last sync pulse
								// Words 2-3 = Sync pulse count
	ushort	peakSumLow;			// Lower 16 bits of Peak Sum
	ushort	group3;				// Offset of Peak, Higher 8 bits of Peak Sum
	ushort	preriseLow;			// Lower 16 bits of Prerise
	ushort	group4;				// Lower 8 bits of integratedSum, Higher 8 bits of Prerise
	ushort	intSumHigh;			// Upper 16 bits of integratedSum
	ushort	baseline;			// Baseline
	ushort	cfdPoint[4];		// CFD Timestamp Interpolation Points
	ushort	intTimestamp[4];	// Internal Timestamp
								// Word 0 = Reserved for interpolation
								// Words 1-2 = 48 bit Timestamp
} Event_Header;

typedef struct _Event_Packet {
	Event_Header	header;
	ushort			waveform[MAX_EVENT_DATA];
} Event_Packet;

typedef struct _Event {
	uint		header;			// 0xAAAAAAAA
	ushort		packetLength;
	ushort		triggerType;
	EventStatus	status;
	uchar		headerType;
	ushort		triggerID;
	ushort		moduleID;
	ushort		channelID;
	uint		syncDelay;
	uint		syncCount;
	int			peakSum;
	char		peakTime;
	uint		prerise;
	uint		integratedSum;
	ushort		baseline;
	short		cfdPoint[4];
	ushort		intTimestamp[4];
	ushort		waveformWords;
	ushort		waveform[MAX_EVENT_DATA];
} Event;

typedef struct _EventInfo {
	int max;
	int min;
	int timingMark[MAX_TIMING_MARKS];
	int timingMarkColor[MAX_TIMING_MARKS];
	double fracTimestamp;
	double intEnergy;
	double intEnergyB;
	double mean;
	double stdDev;
} EventInfo;

typedef struct _LBNEVars {
	uint	commType;
	uint	connected;
	uint	useNoComm;
	uint	useTCPComm;
	uint	useUSBComm;
	uint	useUDPComm;
	uint	numDevices;
	int		currDevice;
	int		panelMain;
	int		panelDigitizer;
	int		panelGeneric;
	int		panelTest;
	int		panelFlash;
	char	hostIP[20];
	char	deviceIP[20];
} LBNEVars;

typedef struct _LBNERegMap {
	// Registers in the ARM Processor			Address
	uint IP4Address;						//	0x00000100
	uint IP4Netmask;						//	0x00000104
	uint IP4Gateway;						//	0x00000108
	uint MACAddressLSB;						//	0x00000110
	uint MACAddressMSB;						//	0x00000114
	uint EthernetReset;						//	0x00000180
	uint RestoreSelect;						//	0x00000200
	uint Restore;   						//	0x00000204
	uint PurgeDDR;							//	0x00000300
	
	// Registers in the Zynq FPGA				Address
	uint eventDataInterfaceSelect;			//	0x40000020
	uint codeErrCounts[5];					//	0x40000100 - 0x40000110
	uint dispErrCounts[5];					//	0x40000120 - 0x40000130
	uint link_rx_status;					//	0x40000140
	uint eventDataControl;					//	0x40000144
	uint eventDataPhaseControl;				//	0x40000148
	uint eventDataPhaseStatus;				//	0x4000014C
	uint c2c_master_status;					//	0x40000150
	uint c2c_control;						//	0x40000154
	uint c2c_master_intr_control;			//	0x40000158
	uint dspStatus;							//	0x40000160
	uint comm_clock_status;					//	0x40000170
	uint comm_clock_control;				//	0x40000174
	uint comm_led_config;					//	0x40000180
	uint comm_led_input;					//	0x40000184
	uint eventDataStatus;					//	0x40000190

	uint qi_dac_control;					//	0x40000200
	uint qi_dac_config;						//	0x40000204

	uint bias_control;						//	0x40000300
	uint bias_status;						//	0x40000304
	uint bias_config[MAX_CHANNELS];			//	0x40000340 - 0x4000036C
	uint bias_readback[MAX_CHANNELS];		//	0x40000380 - 0x400003AC

	uint mon_config;						//	0x40000400
	uint mon_select;						//	0x40000404
	uint mon_gpio;							//	0x40000408
	uint mon_config_readback;				//	0x40000410
	uint mon_select_readback;				//	0x40000414
	uint mon_gpio_readback;					//	0x40000418
	uint mon_id_readback;					//	0x4000041C
	uint mon_control;						//	0x40000420
	uint mon_status;						//	0x40000424
	uint mon_bias[MAX_CHANNELS];			//	0x40000440 - 0x4000046C
	uint mon_value[9];						//	0x40000480 - 0x400004A0

	// Registers in the Artix FPGA
	uint board_id;							//	0x80000000
	uint fifo_control;						//	0x80000004
	uint dsp_clock_status;					//	0x80000020
	uint module_id;							//	0x80000024
	uint c2c_slave_status;					//	0x80000030
	uint c2c_slave_intr_control;			//	0x80000034

	uint channel_control[MAX_CHANNELS];		//	0x80000040 - 0x8000006C
	uint led_threshold[MAX_CHANNELS];		//	0x80000080 - 0x800000AC
	uint cfd_parameters[MAX_CHANNELS];		//	0x800000C0 - 0x800000EC
	uint readout_pretrigger[MAX_CHANNELS];	//	0x80000100 - 0x8000012C
	uint readout_window[MAX_CHANNELS];		//	0x80000140 - 0x8000016C

	uint p_window[MAX_CHANNELS];			//	0x80000180 - 0x800001AC
	uint i2_window[MAX_CHANNELS];			//	0x800001C0 - 0x800001EC
	uint m1_window[MAX_CHANNELS];			//	0x80000200 - 0x8000022C
	uint m2_window[MAX_CHANNELS];			//	0x80000240 - 0x8000026C
	uint d_window[MAX_CHANNELS];			//	0x80000280 - 0x800002AC
	uint i1_window[MAX_CHANNELS];			//	0x800002C0 - 0x800002EC
	uint disc_width[MAX_CHANNELS];			//	0x80000300 - 0x8000032C
	uint baseline_start[MAX_CHANNELS];		//	0x80000340 - 0x8000036C

	uint trigger_input_delay;				//	0x80000400
	uint gpio_output_width;					//	0x80000404
	uint front_panel_config;				//	0x80000408
	uint channel_pulsed_control;			//	0x8000040C
	uint dsp_led_config;					//	0x80000410
	uint dsp_led_input;						//	0x80000414
	uint baseline_delay;					//	0x80000418
	uint diag_channel_input;				//	0x8000041C
	uint dsp_pulsed_control;				//	0x80000420
	uint event_data_control;				//	0x80000424
	uint adc_config;						//	0x80000428
	uint adc_config_load;					//	0x8000042C
	uint qi_config;							//	0x80000430
	uint qi_delay;							//	0x80000434
	uint qi_pulse_width;					//	0x80000438
	uint qi_pulsed;							//	0x8000043C
	uint external_gate_width;				//	0x80000440
	uint lat_timestamp_lsb;					//	0x80000484
	uint lat_timestamp_msb;					//	0x80000488
	uint live_timestamp_lsb;				//	0x8000048C
	uint live_timestamp_msb;				//	0x80000490
	uint sync_period;						//	0x80000494
	uint sync_delay;						//	0x80000498
	uint sync_count;						//	0x8000049C

	uint master_logic_control;				//	0x80000500
	uint overflow_status;					//	0x80000508
	uint phase_value;						//	0x8000050C
	uint link_tx_status;					//	0x80000510
	uint dsp_clock_control;					//	0x80000520
	uint dsp_clock_phase_control;			//	0x80000524
	uint code_revision;						//	0x80000600
	uint code_date;							//	0x80000604

	uint dropped_event_count[MAX_CHANNELS];	//	0x80000700 - 0x8000072C
	uint accepted_event_count[MAX_CHANNELS];//	0x80000740 - 0x8000076C
	uint ahit_count[MAX_CHANNELS];			//	0x80000780 - 0x800007AC
	uint disc_count[MAX_CHANNELS];			//	0x800007C0 - 0x800007EC
	uint idelay_count[MAX_CHANNELS];		//	0x80000800 - 0x8000082C
	uint adc_data_monitor[MAX_CHANNELS];	//	0x80000840 - 0x8000086C
	uint adc_status[MAX_CHANNELS];			//	0x80000880 - 0x800008AC
} LBNERegMap;

//==============================================================================
// Include files
//==============================================================================

#ifdef ANL

#endif

//==============================================================================
// Globally Externed Variables
//==============================================================================

extern LBNERegMap lbneReg;

//==============================================================================
// Function Prototypes
//==============================================================================

#ifdef ANL

#endif

// Event Processing Functions
void LBNE_Init (void);
int  LBNE_EventUnpack(Event_Packet* packet, Event* event);

#endif
