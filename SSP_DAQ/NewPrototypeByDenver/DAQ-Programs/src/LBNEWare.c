//==============================================================================
//
// Title:		LBNEWare.c
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

//==============================================================================
// Include Files
//==============================================================================

#include "LBNEWare.h"
#include "Device.h"

//==============================================================================
// Global Variables
//==============================================================================

LBNEVars	lbne;
LBNERegMap	lbneReg;

//==============================================================================
// External Variables
//==============================================================================

extern Device device[MAX_DEVICES];

//==============================================================================
// Global Functions
//==============================================================================

#ifdef ANL

#endif

//==============================================================================
// Event Processing Functions
//==============================================================================

int LBNE_EventUnpack(Event_Packet* packet, Event* event) {
	int i = 0;
	int error = 0;
	ushort waveformLength = 0;
	ushort packetLength = 0;
	
	// Calculate lengths
	packetLength = packet->header.length;		// Packet in uints 
	packetLength = packetLength * sizeof(uint);	// Packet in bytes
	
	if (packetLength < sizeof(Event_Header)) {
		error = errorEventTooSmall;
	} else {
		waveformLength = packetLength - sizeof(Event_Header);	// Waveform in bytes
		waveformLength = waveformLength >> 1;					// Waveform in shorts
		
		if (waveformLength > MAX_EVENT_DATA) {
			error = errorEventTooLarge;
		}
	}
	
	if (error) {
		// Return empty event structure
		event->header			= 0;
		event->packetLength		= 0;
		event->triggerType		= 0;
		event->status.flags		= 0;
		event->headerType		= 0;
		event->triggerID		= 0;
		event->moduleID			= 0;
		event->channelID		= 0;
		event->syncDelay		= 0;
		event->syncCount		= 0;
		event->peakSum			= 0;
		event->peakTime			= 0;
		event->prerise			= 0;
		event->integratedSum	= 0;
		event->baseline			= 0;
		event->cfdPoint[0]		= 0;
		event->cfdPoint[1]		= 0;
		event->cfdPoint[2]		= 0;
		event->cfdPoint[3]		= 0;
		event->intTimestamp[0]	= 0;
		event->intTimestamp[1]	= 0;
		event->intTimestamp[2]	= 0;
		event->intTimestamp[3]	= 0;
		event->waveformWords	= 0;
		
		for (i = 0; i < MAX_EVENT_DATA; i++) {
			event->waveform[i] = 0;
		}
	} else {
		// Unpack packet into event structure
		event->header			= packet->header.header;
		event->packetLength		= packet->header.length;
		event->triggerType		= (packet->header.group1 & 0xFF00) >> 8;
		event->status.flags		= (packet->header.group1 & 0x00F0) >> 4;
		event->headerType		= (packet->header.group1 & 0x000F) >> 0;
		event->triggerID		= packet->header.triggerID;
		event->moduleID			= (packet->header.group2 & 0xFFF0) >> 4;
		event->channelID		= (packet->header.group2 & 0x000F) >> 0;
		event->syncDelay		= ((uint)(packet->header.timestamp[1]) << 16) + (uint)(packet->header.timestamp[0]);
		event->syncCount		= ((uint)(packet->header.timestamp[3]) << 16) + (uint)(packet->header.timestamp[2]);
		// PeakSum arrives as a signed 24 bit number.  It must be sign extended when stored as a 32 bit int
		event->peakSum			= ((packet->header.group3 & 0x00FF) << 16) + packet->header.peakSumLow;
		if (event->peakSum & 0x00800000) {
			event->peakSum |= 0xFF000000;
		}
		event->peakTime			= (packet->header.group3 & 0xFF00) >> 8;
		event->prerise			= ((packet->header.group4 & 0x00FF) << 16) + packet->header.preriseLow;
		event->integratedSum	= ((uint)(packet->header.intSumHigh) << 8) + (((uint)(packet->header.group4) & 0xFF00) >> 8);
		event->baseline			= packet->header.baseline;
		event->cfdPoint[0]		= packet->header.cfdPoint[0];
		event->cfdPoint[1]		= packet->header.cfdPoint[1];
		event->cfdPoint[2]		= packet->header.cfdPoint[2];
		event->cfdPoint[3]		= packet->header.cfdPoint[3];
		event->intTimestamp[0]	= packet->header.intTimestamp[0];
		event->intTimestamp[1]	= packet->header.intTimestamp[1];
		event->intTimestamp[2]	= packet->header.intTimestamp[2];
		event->intTimestamp[3]	= packet->header.intTimestamp[3];
		event->waveformWords	= waveformLength;

		// Copy waveform into event structure
		for (i = 0; i < event->waveformWords; i++) {
			event->waveform[i] = packet->waveform[i];
		}
		// Fill rest of waveform storage with zero
		for (; i < MAX_EVENT_DATA; i++) {
			event->waveform[i] = 0;
		}
	}
	
	return error;
}

void LBNE_Init (void)
{
	// NOTE: All comments about default values, read masks, and write masks are current as of 2/11/2014

	// Registers in the Zynq ARM	Address				Address		Default Value	Read Mask		Write Mask		Code Name
	lbneReg.IP4Address				= 0x00000100;
	lbneReg.IP4Netmask				= 0x00000104;
	lbneReg.IP4Gateway				= 0x00000108;
	lbneReg.MACAddressLSB			= 0x00000110;
	lbneReg.MACAddressMSB			= 0x00000114;
	lbneReg.EthernetReset			= 0x00000180;
	lbneReg.RestoreSelect			= 0x00000200;
	lbneReg.Restore					= 0x00000204;
	lbneReg.PurgeDDR				= 0x00000300;
	
	
	// Registers in the Zynq FPGA	Address				Address		Default Value	Read Mask		Write Mask		VHDL Name
	lbneReg.eventDataInterfaceSelect= 0x40000020;	//	X"000",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_test_0

	lbneReg.codeErrCounts[0]		= 0x40000100;	//	X"100",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_code_err_counts(0)
	lbneReg.codeErrCounts[1]		= 0x40000104;	//	X"104",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_code_err_counts(1)
	lbneReg.codeErrCounts[2]		= 0x40000108;	//	X"108",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_code_err_counts(2)
	lbneReg.codeErrCounts[3]		= 0x4000010C;	//	X"10C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_code_err_counts(3)
	lbneReg.codeErrCounts[4]		= 0x40000110;	//	X"110",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_code_err_counts(4)
	lbneReg.dispErrCounts[0]		= 0x40000120;	//	X"120",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_disp_err_counts(0)
	lbneReg.dispErrCounts[1]		= 0x40000124;	//	X"124",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_disp_err_counts(1)
	lbneReg.dispErrCounts[2]		= 0x40000128;	//	X"128",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_disp_err_counts(2)
	lbneReg.dispErrCounts[3]		= 0x4000012C;	//	X"12C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_disp_err_counts(3)
	lbneReg.dispErrCounts[4]		= 0x40000130;	//	X"130",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_disp_err_counts(4)
	lbneReg.link_rx_status			= 0x40000140;	//	X"140",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_link_rx_status
	lbneReg.eventDataControl		= 0x40000144;	//	X"144",		X"0020001F",	X"FFFFFFFF",	X"0033001F",	reg_event_data_control
	lbneReg.eventDataPhaseControl	= 0x40000148;	//	X"148",		X"00000000",	X"00000000",	X"00000003",	reg_event_data_phase_control
	lbneReg.eventDataPhaseStatus	= 0x4000014C;	//	X"14C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_event_data_phase_status
	lbneReg.c2c_master_status		= 0x40000150;	//	X"150",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_c2c_master_status
	lbneReg.c2c_control				= 0x40000154;	//	X"154",		X"00000007",	X"FFFFFFFF",	X"00000007",	reg_c2c_control
	lbneReg.c2c_master_intr_control	= 0x40000158;	//	X"158",		X"00000000",	X"FFFFFFFF",	X"0000000F",	reg_c2c_master_intr_control
	lbneReg.dspStatus				= 0x40000160;	//	X"160",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_dsp_status
	lbneReg.comm_clock_status		= 0x40000170;	//	X"170",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_comm_clock_status
	lbneReg.comm_clock_control		= 0x40000174;	//	X"174",		X"00000001",	X"FFFFFFFF",	X"00000001",	reg_comm_clock_control
	lbneReg.comm_led_config			= 0x40000180;	//	X"180",		X"00000000",	X"FFFFFFFF",	X"00000013",	reg_comm_led_config
	lbneReg.comm_led_input			= 0x40000184;	//	X"184",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_comm_led_input
	lbneReg.eventDataStatus			= 0x40000190;	//	X"190",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_event_data_status

	lbneReg.qi_dac_control			= 0x40000200;	//	X"200",		X"00000000",	X"00000000",	X"00000001",	reg_qi_dac_control
	lbneReg.qi_dac_config			= 0x40000204;	//	X"204",		X"00008000",	X"0003FFFF",	X"0003FFFF",	reg_qi_dac_config
	
	lbneReg.bias_control			= 0x40000300;	//	X"300",		X"00000000",	X"00000000",	X"00000001",	reg_bias_control
	lbneReg.bias_status				= 0x40000304;	//	X"304",		X"00000000",	X"00000FFF",	X"00000000",	regin_bias_status

	lbneReg.bias_config[0]			= 0x40000340;	//	X"340",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(0)
	lbneReg.bias_config[1]			= 0x40000344;	//	X"344",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(1)
	lbneReg.bias_config[2]			= 0x40000348;	//	X"348",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(2)
	lbneReg.bias_config[3]			= 0x4000034C;	//	X"34C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(3)
	lbneReg.bias_config[4]			= 0x40000350;	//	X"350",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(4)
	lbneReg.bias_config[5]			= 0x40000354;	//	X"354",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(5)
	lbneReg.bias_config[6]			= 0x40000358;	//	X"358",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(6)
	lbneReg.bias_config[7]			= 0x4000035C;	//	X"35C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(7)
	lbneReg.bias_config[8]			= 0x40000360;	//	X"360",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(8)
	lbneReg.bias_config[9]			= 0x40000364;	//	X"364",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(9)
	lbneReg.bias_config[10]			= 0x40000368;	//	X"368",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(10)
	lbneReg.bias_config[11]			= 0x4000036C;	//	X"36C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_bias_dac_config(11)

	lbneReg.bias_readback[0]		= 0x40000380;	//	X"380",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(0)
	lbneReg.bias_readback[1]		= 0x40000384;	//	X"384",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(1)
	lbneReg.bias_readback[2]		= 0x40000388;	//	X"388",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(2)
	lbneReg.bias_readback[3]		= 0x4000038C;	//	X"38C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(3)
	lbneReg.bias_readback[4]		= 0x40000390;	//	X"390",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(4)
	lbneReg.bias_readback[5]		= 0x40000394;	//	X"394",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(5)
	lbneReg.bias_readback[6]		= 0x40000398;	//	X"398",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(6)
	lbneReg.bias_readback[7]		= 0x4000039C;	//	X"39C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(7)
	lbneReg.bias_readback[8]		= 0x400003A0;	//	X"3A0",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(8)
	lbneReg.bias_readback[9]		= 0x400003A4;	//	X"3A4",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(9)
	lbneReg.bias_readback[10]		= 0x400003A8;	//	X"3A8",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(10)
	lbneReg.bias_readback[11]		= 0x400003AC;	//	X"3AC",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_bias_dac_readback(11)

	lbneReg.mon_config				= 0x40000400;	//	X"400",		X"0012F000",	X"00FFFFFF",	X"00FFFFFF",	reg_mon_config
	lbneReg.mon_select				= 0x40000404;	//	X"404",		X"00FFFF00",	X"FFFFFFFF",	X"FFFFFFFF",	reg_mon_select
	lbneReg.mon_gpio				= 0x40000408;	//	X"408",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_mon_gpio
	lbneReg.mon_config_readback		= 0x40000410;	//	X"410",		X"00000000",	X"00FFFFFF",	X"00000000",	regin_mon_config_readback
	lbneReg.mon_select_readback		= 0x40000414;	//	X"414",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_select_readback
	lbneReg.mon_gpio_readback		= 0x40000418;	//	X"418",		X"00000000",	X"0000FFFF",	X"00000000",	regin_mon_gpio_readback
	lbneReg.mon_id_readback			= 0x4000041C;	//	X"41C",		X"00000000",	X"000000FF",	X"00000000",	regin_mon_id_readback
	lbneReg.mon_control				= 0x40000420;	//	X"420",		X"00000000",	X"00010100",	X"00010001",	reg_mon_control & regin_mon_control
	lbneReg.mon_status				= 0x40000424;	//	X"424",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_status
	
	lbneReg.mon_bias[0]				= 0x40000440;	//	X"440",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(0)
	lbneReg.mon_bias[1]				= 0x40000444;	//	X"444",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(1)
	lbneReg.mon_bias[2]				= 0x40000448;	//	X"448",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(2)
	lbneReg.mon_bias[3]				= 0x4000044C;	//	X"44C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(3)
	lbneReg.mon_bias[4]				= 0x40000450;	//	X"450",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(4)
	lbneReg.mon_bias[5]				= 0x40000454;	//	X"454",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(5)
	lbneReg.mon_bias[6]				= 0x40000458;	//	X"458",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(6)
	lbneReg.mon_bias[7]				= 0x4000045C;	//	X"45C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(7)
	lbneReg.mon_bias[8]				= 0x40000460;	//	X"460",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(8)
	lbneReg.mon_bias[9]				= 0x40000464;	//	X"464",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(9)
	lbneReg.mon_bias[10]			= 0x40000468;	//	X"468",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(10)
	lbneReg.mon_bias[11]			= 0x4000046C;	//	X"46C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_bias(11)

	lbneReg.mon_value[0]			= 0x40000480;	//	X"480",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(0)
	lbneReg.mon_value[1]			= 0x40000484;	//	X"484",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(1)
	lbneReg.mon_value[2]			= 0x40000488;	//	X"488",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(2)
	lbneReg.mon_value[3]			= 0x4000048C;	//	X"48C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(3)
	lbneReg.mon_value[4]			= 0x40000490;	//	X"490",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(4)
	lbneReg.mon_value[5]			= 0x40000494;	//	X"494",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(5)
	lbneReg.mon_value[6]			= 0x40000498;	//	X"498",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(6)
	lbneReg.mon_value[7]			= 0x4000049C;	//	X"49C",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(7)
	lbneReg.mon_value[8]			= 0x400004A0;	//	X"4A0",		X"00000000",	X"FFFFFFFF",	X"00000000",	regin_mon_value(8)

	// Registers in the Artix FPGA	Address				Address		Default Value	Read Mask		Write Mask		VHDL Name
	lbneReg.board_id				= 0x80000000;	//	X"000",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_board_id
	lbneReg.fifo_control			= 0x80000004;	//	X"004",		X"00000000",	X"0FFFFFFF",	X"08000000",	reg_fifo_control
	lbneReg.dsp_clock_status		= 0x80000020;	//	X"020",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_mmcm_status
	lbneReg.module_id				= 0x80000024;	//	X"024",		X"00000000",	X"00000FFF",	X"00000FFF",	reg_module_id
	lbneReg.c2c_slave_status		= 0x80000030;	//	X"030",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_c2c_slave_status
	lbneReg.c2c_slave_intr_control	= 0x80000034;	//	X"034",		X"00000000",	X"FFFFFFFF",	X"0000000F",	reg_c2c_slave_intr_control

	lbneReg.channel_control[0]		= 0x80000040;	//	X"040",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(0)
	lbneReg.channel_control[1]		= 0x80000044;	//	X"044",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(1)
	lbneReg.channel_control[2]		= 0x80000048;	//	X"048",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(2)
	lbneReg.channel_control[3]		= 0x8000004C;	//	X"04C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(3)
	lbneReg.channel_control[4]		= 0x80000050;	//	X"050",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(4)
	lbneReg.channel_control[5]		= 0x80000054;	//	X"054",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(5)
	lbneReg.channel_control[6]		= 0x80000058;	//	X"058",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(6)
	lbneReg.channel_control[7]		= 0x8000005C;	//	X"05C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(7)
	lbneReg.channel_control[8]		= 0x80000060;	//	X"060",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(8)
	lbneReg.channel_control[9]		= 0x80000064;	//	X"064",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(9)
	lbneReg.channel_control[10]		= 0x80000068;	//	X"068",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(10)
	lbneReg.channel_control[11]		= 0x8000006C;	//	X"06C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_channel_control(11)

	lbneReg.led_threshold[0]		= 0x80000080;	//	X"080",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(0)
	lbneReg.led_threshold[1]		= 0x80000084;	//	X"084",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(1)
	lbneReg.led_threshold[2]		= 0x80000088;	//	X"088",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(2)
	lbneReg.led_threshold[3]		= 0x8000008C;	//	X"08C",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(3)
	lbneReg.led_threshold[4]		= 0x80000090;	//	X"090",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(4)
	lbneReg.led_threshold[5]		= 0x80000094;	//	X"094",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(5)
	lbneReg.led_threshold[6]		= 0x80000098;	//	X"098",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(6)
	lbneReg.led_threshold[7]		= 0x8000009C;	//	X"09C",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(7)
	lbneReg.led_threshold[8]		= 0x800000A0;	//	X"0A0",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(8)
	lbneReg.led_threshold[9]		= 0x800000A4;	//	X"0A4",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(9)
	lbneReg.led_threshold[10]		= 0x800000A8;	//	X"0A8",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(10)
	lbneReg.led_threshold[11]		= 0x800000AC;	//	X"0AC",		X"00000064",	X"00FFFFFF",	X"00FFFFFF",	reg_led_threshold(11)

	lbneReg.cfd_parameters[0]		= 0x800000C0;	//	X"0C0",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(0)
	lbneReg.cfd_parameters[1]		= 0x800000C4;	//	X"0C4",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(1)
	lbneReg.cfd_parameters[2]		= 0x800000C8;	//	X"0C8",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(2)
	lbneReg.cfd_parameters[3]		= 0x800000CC;	//	X"0CC",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(3)
	lbneReg.cfd_parameters[4]		= 0x800000D0;	//	X"0D0",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(4)
	lbneReg.cfd_parameters[5]		= 0x800000D4;	//	X"0D4",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(5)
	lbneReg.cfd_parameters[6]		= 0x800000D8;	//	X"0D8",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(6)
	lbneReg.cfd_parameters[7]		= 0x800000DC;	//	X"0DC",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(7)
	lbneReg.cfd_parameters[8]		= 0x800000E0;	//	X"0E0",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(8)
	lbneReg.cfd_parameters[9]		= 0x800000E4;	//	X"0E4",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(9)
	lbneReg.cfd_parameters[10]		= 0x800000E8;	//	X"0E8",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(10)
	lbneReg.cfd_parameters[11]		= 0x800000EC;	//	X"0EC",		X"00001800",	X"00001FFF",	X"00001FFF",	reg_cfd_parameters(11)

	lbneReg.readout_pretrigger[0]	= 0x80000100;	//	X"100",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(0)
	lbneReg.readout_pretrigger[1]	= 0x80000104;	//	X"104",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(1)
	lbneReg.readout_pretrigger[2]	= 0x80000108;	//	X"108",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(2)
	lbneReg.readout_pretrigger[3]	= 0x8000010C;	//	X"10C",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(3)
	lbneReg.readout_pretrigger[4]	= 0x80000110;	//	X"110",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(4)
	lbneReg.readout_pretrigger[5]	= 0x80000114;	//	X"114",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(5)
	lbneReg.readout_pretrigger[6]	= 0x80000118;	//	X"118",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(6)
	lbneReg.readout_pretrigger[7]	= 0x8000011C;	//	X"11C",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(7)
	lbneReg.readout_pretrigger[8]	= 0x80000120;	//	X"120",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(8)
	lbneReg.readout_pretrigger[9]	= 0x80000124;	//	X"124",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(9)
	lbneReg.readout_pretrigger[10]	= 0x80000128;	//	X"128",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(10)
	lbneReg.readout_pretrigger[11]	= 0x8000012C;	//	X"12C",		X"00000019",	X"000007FF",	X"000007FF",	reg_readout_pretrigger(11)

	lbneReg.readout_window[0]		= 0x80000140;	//	X"140",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(0)
	lbneReg.readout_window[1]		= 0x80000144;	//	X"144",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(1)
	lbneReg.readout_window[2]		= 0x80000148;	//	X"148",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(2)
	lbneReg.readout_window[3]		= 0x8000014C;	//	X"14C",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(3)
	lbneReg.readout_window[4]		= 0x80000150;	//	X"150",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(4)
	lbneReg.readout_window[5]		= 0x80000154;	//	X"154",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(5)
	lbneReg.readout_window[6]		= 0x80000158;	//	X"158",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(6)
	lbneReg.readout_window[7]		= 0x8000015C;	//	X"15C",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(7)
	lbneReg.readout_window[8]		= 0x80000160;	//	X"160",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(8)
	lbneReg.readout_window[9]		= 0x80000164;	//	X"164",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(9)
	lbneReg.readout_window[10]		= 0x80000168;	//	X"168",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(10)
	lbneReg.readout_window[11]		= 0x8000016C;	//	X"16C",		X"000000C8",	X"000007FE",	X"000007FE",	reg_readout_window(11)

	lbneReg.p_window[0]				= 0x80000180;	//	X"180",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(0)
	lbneReg.p_window[1]				= 0x80000184;	//	X"184",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(1)
	lbneReg.p_window[2]				= 0x80000188;	//	X"188",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(2)
	lbneReg.p_window[3]				= 0x8000018C;	//	X"18C",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(3)
	lbneReg.p_window[4]				= 0x80000190;	//	X"190",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(4)
	lbneReg.p_window[5]				= 0x80000194;	//	X"194",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(5)
	lbneReg.p_window[6]				= 0x80000198;	//	X"198",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(6)
	lbneReg.p_window[7]				= 0x8000019C;	//	X"19C",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(7)
	lbneReg.p_window[8]				= 0x800001A0;	//	X"1A0",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(8)
	lbneReg.p_window[9]				= 0x800001A4;	//	X"1A4",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(9)
	lbneReg.p_window[10]			= 0x800001A8;	//	X"1A8",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(10)
	lbneReg.p_window[11]			= 0x800001AC;	//	X"1AC",		X"00000000",	X"000003FF",	X"000003FF",	reg_p_window(11)

	lbneReg.i2_window[0]			= 0x800001C0;	//	X"1C0",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(0)
	lbneReg.i2_window[1]			= 0x800001C4;	//	X"1C4",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(1)
	lbneReg.i2_window[2]			= 0x800001C8;	//	X"1C8",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(2)
	lbneReg.i2_window[3]			= 0x800001CC;	//	X"1CC",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(3)
	lbneReg.i2_window[4]			= 0x800001D0;	//	X"1D0",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(4)
	lbneReg.i2_window[5]			= 0x800001D4;	//	X"1D4",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(5)
	lbneReg.i2_window[6]			= 0x800001D8;	//	X"1D8",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(6)
	lbneReg.i2_window[7]			= 0x800001DC;	//	X"1DC",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(7)
	lbneReg.i2_window[8]			= 0x800001E0;	//	X"1E0",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(8)
	lbneReg.i2_window[9]			= 0x800001E4;	//	X"1E4",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(9)
	lbneReg.i2_window[10]			= 0x800001E8;	//	X"1E8",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(10)
	lbneReg.i2_window[11]			= 0x800001EC;	//	X"1EC",		X"00000014",	X"000003FF",	X"000003FF",	reg_i2_window(11)

	lbneReg.m1_window[0]			= 0x80000200;	//	X"200",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(0)
	lbneReg.m1_window[1]			= 0x80000204;	//	X"204",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(1)
	lbneReg.m1_window[2]			= 0x80000208;	//	X"208",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(2)
	lbneReg.m1_window[3]			= 0x8000020C;	//	X"20C",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(3)
	lbneReg.m1_window[4]			= 0x80000210;	//	X"210",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(4)
	lbneReg.m1_window[5]			= 0x80000214;	//	X"214",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(5)
	lbneReg.m1_window[6]			= 0x80000218;	//	X"218",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(6)
	lbneReg.m1_window[7]			= 0x8000021C;	//	X"21C",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(7)
	lbneReg.m1_window[8]			= 0x80000220;	//	X"220",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(8)
	lbneReg.m1_window[9]			= 0x80000224;	//	X"224",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(9)
	lbneReg.m1_window[10]			= 0x80000228;	//	X"228",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(10)
	lbneReg.m1_window[11]			= 0x8000022C;	//	X"22C",		X"0000000A",	X"000003FF",	X"000003FF",	reg_m1_window(11)

	lbneReg.m2_window[0]			= 0x80000240;	//	X"240",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(0)
	lbneReg.m2_window[1]			= 0x80000244;	//	X"244",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(1)
	lbneReg.m2_window[2]			= 0x80000248;	//	X"248",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(2)
	lbneReg.m2_window[3]			= 0x8000024C;	//	X"24C",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(3)
	lbneReg.m2_window[4]			= 0x80000250;	//	X"250",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(4)
	lbneReg.m2_window[5]			= 0x80000254;	//	X"254",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(5)
	lbneReg.m2_window[6]			= 0x80000258;	//	X"258",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(6)
	lbneReg.m2_window[7]			= 0x8000025C;	//	X"25C",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(7)
	lbneReg.m2_window[8]			= 0x80000260;	//	X"260",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(8)
	lbneReg.m2_window[9]			= 0x80000264;	//	X"264",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(9)
	lbneReg.m2_window[10]			= 0x80000268;	//	X"268",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(10)
	lbneReg.m2_window[11]			= 0x8000026C;	//	X"26C",		X"00000014",	X"0000007F",	X"0000007F",	reg_m2_window(11)

	lbneReg.d_window[0]				= 0x80000280;	//	X"280",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(0)
	lbneReg.d_window[1]				= 0x80000284;	//	X"284",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(1)
	lbneReg.d_window[2]				= 0x80000288;	//	X"288",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(2)
	lbneReg.d_window[3]				= 0x8000028C;	//	X"28C",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(3)
	lbneReg.d_window[4]				= 0x80000290;	//	X"290",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(4)
	lbneReg.d_window[5]				= 0x80000294;	//	X"294",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(5)
	lbneReg.d_window[6]				= 0x80000298;	//	X"298",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(6)
	lbneReg.d_window[7]				= 0x8000029C;	//	X"29C",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(7)
	lbneReg.d_window[8]				= 0x800002A0;	//	X"2A0",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(8)
	lbneReg.d_window[9]				= 0x800002A4;	//	X"2A4",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(9)
	lbneReg.d_window[10]			= 0x800002A8;	//	X"2A8",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(10)
	lbneReg.d_window[11]			= 0x800002AC;	//	X"2AC",		X"00000014",	X"0000007F",	X"0000007F",	reg_d_window(11)

	lbneReg.i1_window[0]			= 0x800002C0;	//	X"2C0",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(0)
	lbneReg.i1_window[1]			= 0x800002C4;	//	X"2C4",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(1)
	lbneReg.i1_window[2]			= 0x800002C8;	//	X"2C8",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(2)
	lbneReg.i1_window[3]			= 0x800002CC;	//	X"2CC",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(3)
	lbneReg.i1_window[4]			= 0x800002D0;	//	X"2D0",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(4)
	lbneReg.i1_window[5]			= 0x800002D4;	//	X"2D4",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(5)
	lbneReg.i1_window[6]			= 0x800002D8;	//	X"2D8",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(6)
	lbneReg.i1_window[7]			= 0x800002DC;	//	X"2DC",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(7)
	lbneReg.i1_window[8]			= 0x800002E0;	//	X"2E0",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(8)
	lbneReg.i1_window[9]			= 0x800002E4;	//	X"2E4",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(9)
	lbneReg.i1_window[10]			= 0x800002E8;	//	X"2E8",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(10)
	lbneReg.i1_window[11]			= 0x800002EC;	//	X"2EC",		X"00000010",	X"000003FF",	X"000003FF",	reg_i1_window(11)

	lbneReg.disc_width[0]			= 0x80000300;	//	X"300",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_disc_width(0)
	lbneReg.disc_width[1]			= 0x80000304;	//	X"304",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_disc_width(1)
	lbneReg.disc_width[2]			= 0x80000308;	//	X"308",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_disc_width(2)
	lbneReg.disc_width[3]			= 0x8000030C;	//	X"30C",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_disc_width(3)
	lbneReg.disc_width[4]			= 0x80000310;	//	X"310",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_disc_width(4)
	lbneReg.disc_width[5]			= 0x80000314;	//	X"314",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_disc_width(5)
	lbneReg.disc_width[6]			= 0x80000318;	//	X"318",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_disc_width(6)
	lbneReg.disc_width[7]			= 0x8000031C;	//	X"31C",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_disc_width(7)
	lbneReg.disc_width[8]			= 0x80000320;	//	X"320",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_disc_width(8)
	lbneReg.disc_width[9]			= 0x80000324;	//	X"324",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_disc_width(9)
	lbneReg.disc_width[10]			= 0x80000328;	//	X"328",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_disc_width(10)
	lbneReg.disc_width[11]			= 0x8000032C;	//	X"32C",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_disc_width(11)

	lbneReg.baseline_start[0]		= 0x80000340;	//	X"340",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(0)
	lbneReg.baseline_start[1]		= 0x80000344;	//	X"344",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(1)
	lbneReg.baseline_start[2]		= 0x80000348;	//	X"348",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(2)
	lbneReg.baseline_start[3]		= 0x8000034C;	//	X"34C",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(3)
	lbneReg.baseline_start[4]		= 0x80000350;	//	X"350",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(4)
	lbneReg.baseline_start[5]		= 0x80000354;	//	X"354",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(5)
	lbneReg.baseline_start[6]		= 0x80000358;	//	X"358",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(6)
	lbneReg.baseline_start[7]		= 0x8000035C;	//	X"35C",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(7)
	lbneReg.baseline_start[8]		= 0x80000360;	//	X"360",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(8)
	lbneReg.baseline_start[9]		= 0x80000364;	//	X"364",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(9)
	lbneReg.baseline_start[10]		= 0x80000368;	//	X"368",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(10)
	lbneReg.baseline_start[11]		= 0x8000036C;	//	X"36C",		X"00002000",	X"00003FFF",	X"00003FFF",	reg_baseline_start(11)

	lbneReg.trigger_input_delay		= 0x80000400;	//	X"400",		X"00000010",	X"0000FFFF",	X"0000FFFF",	reg_trigger_input_delay
	lbneReg.gpio_output_width		= 0x80000404;	//	X"404",		X"0000000F",	X"0000FFFF",	X"0000FFFF",	reg_gpio_output_width
	lbneReg.front_panel_config		= 0x80000408;	//	X"408",		X"00001111",	X"00773333",	X"00773333",	reg_front_panel_config
	lbneReg.channel_pulsed_control	= 0x8000040C;	//	X"40C",		X"00000000",	X"00000000",	X"FFFFFFFF",	reg_channel_pulsed_control
	lbneReg.dsp_led_config			= 0x80000410;	//	X"410",		X"00000000",	X"FFFFFFFF",	X"00000003",	reg_dsp_led_config
	lbneReg.dsp_led_input			= 0x80000414;	//	X"414",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_dsp_led_input
	lbneReg.baseline_delay			= 0x80000418;	//	X"418",		X"00000019",	X"000000FF",	X"000000FF",	reg_baseline_delay
	lbneReg.diag_channel_input		= 0x8000041C;	//	X"41C",		X"00000000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_diag_channel_input
	lbneReg.dsp_pulsed_control		= 0x80000420;	//	X"420",		X"00000000",	X"00000000",	X"FFFFFFFF",	reg_dsp_pulsed_control
	lbneReg.event_data_control		= 0x80000424;	//	X"424",		X"00000001",	X"00020001",	X"00020001",	reg_event_data_control
	lbneReg.adc_config				= 0x80000428;	//	X"428",		X"00010000",	X"FFFFFFFF",	X"FFFFFFFF",	reg_adc_config
	lbneReg.adc_config_load			= 0x8000042C;	//	X"42C",		X"00000000",	X"00000000",	X"00000001",	reg_adc_config_load
	lbneReg.qi_config				= 0x80000430;	//	X"430",		X"0FFF1700",	X"0FFF1F11",	X"0FFF1F11",	reg_qi_config
	lbneReg.qi_delay				= 0x80000434;	//	X"434",		X"00000000",	X"0000007F",	X"0000007F",	reg_qi_delay
	lbneReg.qi_pulse_width			= 0x80000438;	//	X"438",		X"00000000",	X"0000FFFF",	X"0000FFFF",	reg_qi_pulse_width
	lbneReg.qi_pulsed				= 0x8000043C;	//	X"43C",		X"00000000",	X"00000000",	X"00030001",	reg_qi_pulsed
	lbneReg.external_gate_width		= 0x80000440;	//	X"440",		X"00008000",	X"0000FFFF",	X"0000FFFF",	reg_external_gate_width
	lbneReg.lat_timestamp_lsb		= 0x80000484;	//	X"484",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_lat_timestamp (lsb)
	lbneReg.lat_timestamp_msb		= 0x80000488;	//	X"488",		X"00000000",	X"0000FFFF",	X"00000000",	reg_lat_timestamp (msb)
	lbneReg.live_timestamp_lsb		= 0x8000048C;	//	X"48C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_live_timestamp (lsb)
	lbneReg.live_timestamp_msb		= 0x80000490;	//	X"490",		X"00000000",	X"0000FFFF",	X"00000000",	reg_live_timestamp (msb)
	lbneReg.sync_period				= 0x80000494;	//	X"494",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_last_sync_reset_count
	lbneReg.sync_delay				= 0x80000498;	//	X"498",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_external_timestamp (lsb)
	lbneReg.sync_count				= 0x8000049C;	//	X"49C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_external_timestamp (msb)

	lbneReg.master_logic_control	= 0x80000500;	//	X"500",		X"00000000",	X"FFFFFFFF",	X"00000073",	reg_master_logic_control
	lbneReg.overflow_status			= 0x80000508;	//	X"508",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_overflow_status
	lbneReg.phase_value				= 0x8000050C;	//	X"50C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_phase_value
	lbneReg.link_tx_status			= 0x80000510;	//	X"510",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_link_tx_status
	lbneReg.dsp_clock_control		= 0x80000520;	//	X"520",		X"00000000",	X"00000713",	X"00000713",	reg_dsp_clock_control		
    lbneReg.dsp_clock_phase_control	= 0x80000524;	//	X"524",		X"00000000",	X"00000000",	X"00000007",	reg_dsp_clock_phase_control
	
	lbneReg.code_revision			= 0x80000600;	//	X"600",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_code_revision
	lbneReg.code_date				= 0x80000604;	//	X"604",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_code_date

	lbneReg.dropped_event_count[0]	= 0x80000700;	//	X"700",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(0)
	lbneReg.dropped_event_count[1]	= 0x80000704;	//	X"704",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(1)
	lbneReg.dropped_event_count[2]	= 0x80000708;	//	X"708",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(2)
	lbneReg.dropped_event_count[3]	= 0x8000070C;	//	X"70C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(3)
	lbneReg.dropped_event_count[4]	= 0x80000710;	//	X"710",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(4)
	lbneReg.dropped_event_count[5]	= 0x80000714;	//	X"714",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(5)
	lbneReg.dropped_event_count[6]	= 0x80000718;	//	X"718",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(6)
	lbneReg.dropped_event_count[7]	= 0x8000071C;	//	X"71C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(7)
	lbneReg.dropped_event_count[8]	= 0x80000720;	//	X"720",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(8)
	lbneReg.dropped_event_count[9]	= 0x80000724;	//	X"724",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(9)
	lbneReg.dropped_event_count[10]	= 0x80000728;	//	X"728",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(10)
	lbneReg.dropped_event_count[11]	= 0x8000072C;	//	X"72C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_dropped_event_count(11)

	lbneReg.accepted_event_count[0]	= 0x80000740;	//	X"740",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(0)
	lbneReg.accepted_event_count[1]	= 0x80000744;	//	X"744",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(1)
	lbneReg.accepted_event_count[2]	= 0x80000748;	//	X"748",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(2)
	lbneReg.accepted_event_count[3]	= 0x8000074C;	//	X"74C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(3)
	lbneReg.accepted_event_count[4]	= 0x80000750;	//	X"750",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(4)
	lbneReg.accepted_event_count[5]	= 0x80000754;	//	X"754",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(5)
	lbneReg.accepted_event_count[6]	= 0x80000758;	//	X"758",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(6)
	lbneReg.accepted_event_count[7]	= 0x8000075C;	//	X"75C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(7)
	lbneReg.accepted_event_count[8]	= 0x80000760;	//	X"760",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(8)
	lbneReg.accepted_event_count[9]	= 0x80000764;	//	X"764",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(9)
	lbneReg.accepted_event_count[10]= 0x80000768;	//	X"768",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(10)
	lbneReg.accepted_event_count[11]= 0x8000076C;	//	X"76C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_accepted_event_count(11)

	lbneReg.ahit_count[0]			= 0x80000780;	//	X"780",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(0)
	lbneReg.ahit_count[1]			= 0x80000784;	//	X"784",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(1)
	lbneReg.ahit_count[2]			= 0x80000788;	//	X"788",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(2)
	lbneReg.ahit_count[3]			= 0x8000078C;	//	X"78C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(3)
	lbneReg.ahit_count[4]			= 0x80000790;	//	X"790",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(4)
	lbneReg.ahit_count[5]			= 0x80000794;	//	X"794",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(5)
	lbneReg.ahit_count[6]			= 0x80000798;	//	X"798",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(6)
	lbneReg.ahit_count[7]			= 0x8000079C;	//	X"79C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(7)
	lbneReg.ahit_count[8]			= 0x800007A0;	//	X"7A0",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(8)
	lbneReg.ahit_count[9]			= 0x800007A4;	//	X"7A4",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(9)
	lbneReg.ahit_count[10]			= 0x800007A8;	//	X"7A8",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(10)
	lbneReg.ahit_count[11]			= 0x800007AC;	//	X"7AC",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_ahit_count(11)

	lbneReg.disc_count[0]			= 0x800007C0;	//	X"7C0",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(0)
	lbneReg.disc_count[1]			= 0x800007C4;	//	X"7C4",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(1)
	lbneReg.disc_count[2]			= 0x800007C8;	//	X"7C8",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(2)
	lbneReg.disc_count[3]			= 0x800007CC;	//	X"7CC",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(3)
	lbneReg.disc_count[4]			= 0x800007D0;	//	X"7D0",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(4)
	lbneReg.disc_count[5]			= 0x800007D4;	//	X"7D4",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(5)
	lbneReg.disc_count[6]			= 0x800007D8;	//	X"7D8",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(6)
	lbneReg.disc_count[7]			= 0x800007DC;	//	X"7DC",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(7)
	lbneReg.disc_count[8]			= 0x800007E0;	//	X"7E0",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(8)
	lbneReg.disc_count[9]			= 0x800007E4;	//	X"7E4",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(9)
	lbneReg.disc_count[10]			= 0x800007E8;	//	X"7E8",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(10)
	lbneReg.disc_count[11]			= 0x800007EC;	//	X"7EC",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_disc_count(11)

	lbneReg.idelay_count[0]			= 0x80000800;	//	X"800",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(0)
	lbneReg.idelay_count[1]			= 0x80000804;	//	X"804",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(1)
	lbneReg.idelay_count[2]			= 0x80000808;	//	X"808",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(2)
	lbneReg.idelay_count[3]			= 0x8000080C;	//	X"80C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(3)
	lbneReg.idelay_count[4]			= 0x80000810;	//	X"810",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(4)
	lbneReg.idelay_count[5]			= 0x80000814;	//	X"814",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(5)
	lbneReg.idelay_count[6]			= 0x80000818;	//	X"818",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(6)
	lbneReg.idelay_count[7]			= 0x8000081C;	//	X"81C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(7)
	lbneReg.idelay_count[8]			= 0x80000820;	//	X"820",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(8)
	lbneReg.idelay_count[9]			= 0x80000824;	//	X"824",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(9)
	lbneReg.idelay_count[10]		= 0x80000828;	//	X"828",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(10)
	lbneReg.idelay_count[11]		= 0x8000082C;	//	X"82C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_idelay_count(11)

	lbneReg.adc_data_monitor[0]		= 0x80000840;	//	X"840",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(0)
	lbneReg.adc_data_monitor[1]		= 0x80000844;	//	X"844",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(1)
	lbneReg.adc_data_monitor[2]		= 0x80000848;	//	X"848",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(2)
	lbneReg.adc_data_monitor[3]		= 0x8000084C;	//	X"84C",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(3)
	lbneReg.adc_data_monitor[4]		= 0x80000850;	//	X"850",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(4)
	lbneReg.adc_data_monitor[5]		= 0x80000854;	//	X"854",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(5)
	lbneReg.adc_data_monitor[6]		= 0x80000858;	//	X"858",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(6)
	lbneReg.adc_data_monitor[7]		= 0x8000085C;	//	X"85C",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(7)
	lbneReg.adc_data_monitor[8]		= 0x80000860;	//	X"860",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(8)
	lbneReg.adc_data_monitor[9]		= 0x80000864;	//	X"864",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(9)
	lbneReg.adc_data_monitor[10]	= 0x80000868;	//	X"868",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(10)
	lbneReg.adc_data_monitor[11]	= 0x8000086C;	//	X"86C",		X"00000000",	X"0000FFFF",	X"00000000",	reg_adc_data_monitor(11)

	lbneReg.adc_status[0]			= 0x80000880;	//	X"880",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(0)
	lbneReg.adc_status[1]			= 0x80000884;	//	X"884",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(1)
	lbneReg.adc_status[2]			= 0x80000888;	//	X"888",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(2)
	lbneReg.adc_status[3]			= 0x8000088C;	//	X"88C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(3)
	lbneReg.adc_status[4]			= 0x80000890;	//	X"890",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(4)
	lbneReg.adc_status[5]			= 0x80000894;	//	X"894",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(5)
	lbneReg.adc_status[6]			= 0x80000898;	//	X"898",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(6)
	lbneReg.adc_status[7]			= 0x8000089C;	//	X"89C",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(7)
	lbneReg.adc_status[8]			= 0x800008A0;	//	X"8A0",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(8)
	lbneReg.adc_status[9]			= 0x800008A4;	//	X"8A4",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(9)
	lbneReg.adc_status[10]			= 0x800008A8;	//	X"8A8",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(10)
	lbneReg.adc_status[11]			= 0x800008AC;	//	X"8AC",		X"00000000",	X"FFFFFFFF",	X"00000000",	reg_adc_status(11)
}

