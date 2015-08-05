// Configure and run Digitizer(s) in free run mode

// C++ includes
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <vector>
#include <time.h>

// ANL includes
#include "LBNEWare.h"
#include "Device.h"
#include "ftd2xx.h"

// ROOT includes
#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TH2.h>
#include <TEllipse.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TApplication.h>
#include <TROOT.h>

// Main Program
int main( int nArgs, char **args ) {

  // Read command line parameters
  if ( nArgs < 3 ) {
    std::cout << "Please specify output filename and run time (seconds)." << std::endl;
    return 1;
  }
  std::string outFileName(args[1]);
  int runTime(30);
  { std::stringstream tmp; tmp << std::string(args[2]); tmp >> runTime; }
  int board = 0;
  std::cout << "Free run for " << runTime << " seconds: " << outFileName << std::endl;

  // Create the ROOT Application (to draw canvases)
  TApplication *theApp = new TApplication("FreeRun",&nArgs,args);
  //gStyle->SetPalette(1);

  // Configuration for the SSP
  int err(0);
  uint nSamples = 800; // max = 2046 //800 //250
  uint preTrigSamples = 150; //150 //50
  uint leadEdgeThresh[12];
  leadEdgeThresh[ 0] = 65;
  leadEdgeThresh[ 1] = 65;
  leadEdgeThresh[ 2] = 65;
  leadEdgeThresh[ 3] = 65;
  leadEdgeThresh[ 4] = 65;
  leadEdgeThresh[ 5] = 65;
  leadEdgeThresh[ 6] = 65;
  leadEdgeThresh[ 7] = 65;
  leadEdgeThresh[ 8] = 65;
  leadEdgeThresh[ 9] = 65;
  leadEdgeThresh[10] = 6000;
  leadEdgeThresh[11] = 6000;
  /* Trigger/Waveform Polarity Modes
   * Positive Waveform Polarity
   *  Trigger:  external = 0x10F06201, self (negative) = 0x90F00801, self (positive) = 0x90F00401
   * Negative Waveform Polarity
   *  Trigger:  external = 0x00F06001, self (negative) = 0x80F00801, self (positive) = 0x80F00401
   */
  uint controlMode = 0x90F00801;
  uint m1Window = 20;
  uint m2Window = 10;
  uint pWindow = 2;
  uint kWindow = 100;
  uint iWindow = std::min(int(nSamples)-int(preTrigSamples),1023);
  uint dWindow(0); // not sure of default, will read later
  uint biasSetting;

  // Clear all FTDI devices
  DeviceInit();
  LBNE_Init();

  // Locate SSP
  uint nDev(0);
  err = DeviceDiscover(commUSB,&nDev);
  int nDevices = nDev;
  std::cout << "Discovered " << nDevices << " SSPs." << std::endl;
  if ( nDevices == 0 || nDevices > 2 ) {
    std::cout << "DeviceDiscover reports " << nDevices << " devices are connected. Expected 1 or 2, so quitting." << std::endl;
    return 1;
  }

  // Get SSP serial numbers
  std::string SerNum[nDevices];
  uint ModNum[nDevices];
  for ( board = 0; board < nDevices; ++board ) { 
    FT_HANDLE       fControlDevRef;
    char            fDescription[MAX_LENGTH_DESC];
    char            fSerial[MAX_LENGTH_NAME];
    DWORD           fFlags;
    DWORD           fID;
    DWORD           fLocation;
    DWORD           fType;
    FT_GetDeviceInfoDetail(board, &fFlags, &fType, &fID, &fLocation, fSerial, fDescription, &fControlDevRef);
    SerNum[board] = std::string(fSerial);
    if ( SerNum[board] == std::string("LBNE0002A") || SerNum[board] == std::string("LBNE0018B") ) ModNum[board] = 2;
    if ( SerNum[board] == std::string("LBNE0018A") || SerNum[board] == std::string("LBNE0002B") ) ModNum[board] = 18;
    //    std::cout << "Board Number" << board << " - " << "Board Serial: " << SerNum[board] << " - ModNum: "<< ModNum[board] << std::endl;
    //    std::cout << board << " " << fFlags << " " << fType << " " << fID << " " << fLocation << " " << fSerial << " " << fDescription << std::endl; 
    std::cout << "Assigning board " << board << " with serial number " << SerNum[board] << " as module " << ModNum[board] << std::endl;
  }

  for ( board = 0; board < nDevices; ++board) {

    // Connect to SSP
    err = DeviceConnect(commUSB,board,NULL,NULL);
    if ( err != 0 ) {
      std::cout << "Failed to connect to board " << board << ". ErrorCode " << err << std::endl;
      return 1;
    }
    DeviceWrite(lbneReg.eventDataInterfaceSelect, 0x0, board); // use USB interface
    
    // Reset SSP
    err = DeviceStopReset(board);
    if ( err != 0 ) {
      std::cout << "Failed to reset board " << board << ". ErrorCode " << err << std::endl;
      return 1;
    }
    
    // Initialize SSP
    //std::cout << "Setting up board configuration." << std::endl;
    DeviceSetup(board); // Load defaults (defined in Device.c)
    DeviceWrite(lbneReg.module_id,ModNum[board],board);
    // Override default run parameters with our configuration
    for ( int ch = 0; ch < 12; ++ch ) {
      DeviceWrite(lbneReg.readout_window[ch],nSamples,board);
      DeviceWrite(lbneReg.readout_pretrigger[ch],preTrigSamples,board);
      DeviceWrite(lbneReg.channel_control[ch],controlMode,board); // formerly control_status
      DeviceWrite(lbneReg.led_threshold[ch],leadEdgeThresh[ch],board);
      DeviceWrite(lbneReg.m1_window[ch],m1Window,board);
      DeviceWrite(lbneReg.m2_window[ch],m2Window,board);
      DeviceWrite(lbneReg.p_window[ch],pWindow,board);
      DeviceWrite(lbneReg.i2_window[ch],kWindow,board); // formerly k_window
      DeviceWrite(lbneReg.i1_window[ch],iWindow,board); // formerly i_window
      DeviceWriteMask(lbneReg.bias_config[ch],0x00040FFF,0x00040DA4,board); // value & enable  D60 -> 24.5 V on output (formerly bias_dac_value)
    }
    
    // Load channel delays that were just set
    DeviceWrite(lbneReg.channel_pulsed_control,1,board);
    
    if ( err != 0 ) {
      std::cout << "Failed to configure ANL Digitizer." << std::endl;
      return err;
    }
    
  }
  board = 0;

  std::cout << "Collecting " << nSamples/150. << " us waveforms with " << preTrigSamples/150. << " us pre-trigger." << std::endl;

  // Get SSP Digitizer configuration info
  uint eventSize = sizeof(Event_Header) + 2*nSamples; // in bytes
  // Readback to confirm
  DeviceRead(lbneReg.m1_window[0],&m1Window,board);
  DeviceRead(lbneReg.m2_window[0],&m2Window,board);
  DeviceRead(lbneReg.p_window[0],&pWindow,board);
  DeviceRead(lbneReg.i2_window[0],&kWindow,board);
  DeviceRead(lbneReg.i1_window[0],&iWindow,board);
  DeviceRead(lbneReg.d_window[0],&dWindow,board);
  DeviceReadMask(lbneReg.bias_config[0],0x00000FFF,&biasSetting,board);

  // Output File
  TFile *outFile = new TFile(outFileName.c_str(),"RECREATE");

  // Set up output ROOT tree for SSP Digitizer
  TTree *SSPTree = new TTree("SSPTree","SSPTree");
  Event_Packet ArPacket; Event ArEvent;
  SSPTree->Branch("channelID",&(ArEvent.channelID),"channelID/s",sizeof(ushort));
  SSPTree->Branch("moduleID",&(ArEvent.moduleID),"moduleID/s",sizeof(ushort));
  SSPTree->Branch("syncDelay",&(ArEvent.syncDelay),"syncDelay/i",sizeof(uint));
  SSPTree->Branch("syncCount",&(ArEvent.syncCount),"syncCount/i",sizeof(uint));
  SSPTree->Branch("timestamp",&(ArEvent.intTimestamp),"timestamp/l",sizeof(ulong));
  SSPTree->Branch("peakSum",&(ArEvent.peakSum),"peakSum/I",sizeof(int));
  SSPTree->Branch("peakTime",&(ArEvent.peakTime),"peakTime/C");
  SSPTree->Branch("prerise",&(ArEvent.prerise),"prerise/i",sizeof(uint));
  SSPTree->Branch("integratedSum",&(ArEvent.integratedSum),"integratedSum/i",sizeof(uint));
  SSPTree->Branch("baseline",&(ArEvent.baseline),"baseline/s",sizeof(ushort));
  SSPTree->Branch("cfdPoint",&(ArEvent.cfdPoint),"cfdPoint[4]/S",sizeof(short)*4);
  SSPTree->Branch("nSamples",&(ArEvent.waveformWords),"nSamples/s",sizeof(ushort));
  std::stringstream waveformDescr; waveformDescr << "waveform[" << nSamples << "]/s";
  SSPTree->Branch("waveform",&(ArEvent.waveform),waveformDescr.str().c_str(),sizeof(ushort)*nSamples); // Don't forget to set the buffer size for this one!!!

  // Set up configuration ROOT tree
  TTree *configTree = new TTree("configTree","configTree");
  configTree->Branch("runTime",&runTime,"runTime/I");
  configTree->Branch("nSamples",&nSamples,"nSamples/i");
  configTree->Branch("preTrigSamples",&preTrigSamples,"preTrigSamples/i");
  configTree->Branch("threshold",&leadEdgeThresh,"threshold[12]/i");
  configTree->Branch("m1Window",&m1Window,"m1Window/i");
  configTree->Branch("m2Window",&m2Window,"m2Window/i");
  configTree->Branch("pWindow",&pWindow,"pWindow/i");
  configTree->Branch("kWindow",&kWindow,"kWindow/i");
  configTree->Branch("iWindow",&iWindow,"iWindow/i");
  configTree->Branch("dWindow",&dWindow,"dWindow/i");
  configTree->Branch("biasSetting",&biasSetting,"biasSetting/i");
  configTree->Fill();
  configTree->Write();

  // Monitoring histograms
  TH1D *pulseAmp[nDevices][12];
  for ( int board = 0; board < nDevices; ++board ) {
    for ( int chan = 0; chan < 12; ++chan ) {
      std::stringstream histName; histName << "pulseAmp_MOD" << ModNum[board] << "_CH" << chan;
      std::stringstream histTitle; histTitle << "Pulse Amplitudes, SSP Channel " << ModNum[board] << "-" << chan << ";ADC Counts";
      pulseAmp[board][chan] = new TH1D(histName.str().c_str(),histTitle.str().c_str(),2500,0,2500); //changed from 1000,0,1000 by BH
    }    
  }
  TH1D *pulseInt[nDevices][12];
  for ( int board = 0; board < nDevices; ++board ) {
    for ( int chan = 0; chan < 12; ++chan ) {
      std::stringstream histName; histName << "pulseInt_MOD" << ModNum[board] << "_CH" << chan;
      std::stringstream histTitle; histTitle << "Integrated Pulse, SSP Channel " << ModNum[board] << "-" << chan << ";ADC Counts";
      pulseInt[board][chan] = new TH1D(histName.str().c_str(),histTitle.str().c_str(),1000,0,50000);
    }    
  }
  TH2D *WaveHistAll[nDevices][12];
  for ( int board = 0; board < nDevices; ++board ) {
    for ( int chan = 0; chan < 12; ++chan ) {
      std::stringstream timeName; timeName << "WaveHistAll_MOD" << ModNum[board] << "_CH" << chan;
      std::stringstream timeTitle; timeTitle << "All Waveforms, Channel " << ModNum[board] << "-" << chan << ";Time [#mus];Amplitude [ADC]";
      int ADCrange = 4000;
      int ADChi = 16000;
      int ADClo = ADChi - ADCrange;
      WaveHistAll[board][chan] = new TH2D(timeName.str().c_str(),timeTitle.str().c_str(),int(nSamples),-double(preTrigSamples)/150.,(double(nSamples)-double(preTrigSamples))/150.,ADCrange,ADClo,ADChi);
    }
  }
  TH2D *AmpVInt[nDevices][12];
  for ( int board = 0; board < nDevices; ++board ) {
    for ( int chan = 0; chan < 12; ++chan ) {
      std::stringstream timeName; timeName << "AmpVInt_MOD" << ModNum[board] << "_CH" << chan;
      std::stringstream timeTitle; timeTitle << "Prompt Amplitude vs Integrated Signal, Channel " << ModNum[board] << "-" << chan << ";Amplitude [ADC];Integral [ADC]";
      AmpVInt[board][chan] = new TH2D(timeName.str().c_str(),timeTitle.str().c_str(),1000,0,1000,1000,0,50000);
    }
  }

  std::cout << "Starting acquisition." << std::endl;
  // Enable boards
  for ( board = 0; board < nDevices; ++board ) {
    err = DeviceWrite(lbneReg.bias_control,0x1,board); // write 0x1 here to load bias values (formerly bias_dac_loadvalues)
    err = DeviceTimeout(500,board);
  }
  sleep(2);
  for ( board = 0; board < nDevices; ++board ) {
    err = DeviceStart(board);
  }

  // Loop for specified time
  int totArEvts(0);
  time_t tStart(0), tEnd(0);
  time(&tStart); time(&tEnd);
  while ( tEnd - tStart < runTime ) {

    for ( board = 0; board < nDevices; ++board ) {
      //std::cout << "Scanning board " << board << std::endl;

      /*** SSP Digitizer Readout ***/
      uint dataSize(0); err = DeviceQueueStatus(&dataSize,board); // How many full events have been collected?
      if ( err != 0 ) { std::cout << "Failed device queue status check." << std::endl; break; }
      uint nEvts = dataSize / eventSize;
      //std::cout << "  Received " << nEvts << " events (" << dataSize << "/" << eventSize << ")" << std::endl;
      //if ( nEvts > 0 ) std::cout << "Received " << nEvts << " events on the SSP (" << dataSize << " bytes)." << std::endl;
      for ( uint evt = 0; evt < nEvts; ++evt ) {
	totArEvts++;
	uint dataReceived = 0;
	err = DeviceReceive(&ArPacket,&dataReceived,board);
	if ( err != 0 ) { std::cout << "Failed device receive." << std::endl; break; }
	err = LBNE_EventUnpack(&ArPacket,&ArEvent);
	if ( err != 0 ) { std::cout << "Failed to unpack data." << std::endl; break; }
	SSPTree->Fill();
	//std::cout << board << " " << ArEvent.moduleID << " " << ArEvent.channelID << std::endl;
	if ( ArEvent.channelID < 0 || ArEvent.channelID > 11 ) break;
	pulseAmp[board][ArEvent.channelID]->Fill(ArEvent.prerise/double(kWindow)-ArEvent.peakSum/double(m1Window));
	pulseInt[board][ArEvent.channelID]->Fill(ArEvent.prerise/double(kWindow)*double(iWindow)-ArEvent.integratedSum);
	AmpVInt[board][ArEvent.channelID]->Fill(ArEvent.prerise/double(kWindow)-ArEvent.peakSum/double(m1Window),ArEvent.prerise/double(kWindow)*double(iWindow)-ArEvent.integratedSum);
	// Loop through waveform
	for ( uint samp = 0; samp < nSamples; ++samp ) {
	  double dtTrig = ( double(samp) - double(preTrigSamples) + 0.5 ) / 150.;
	  WaveHistAll[board][ArEvent.channelID]->Fill( dtTrig, ArEvent.waveform[samp] + 0.5 );
	}
      }
      
    }

    time(&tEnd);
  }
  std::cout << "Finished acquisition." << std::endl;

  //// Finalize

  // Finalize SSP
  for ( board = 0; board < nDevices; ++board ) {
    err = DeviceStopReset(board);
    if ( err != 0 ) {
      std::cout << "Failed to stop and reset board " << board << ". ErrorCode " << err << std::endl;
      return 1;
    }
    // Set internal bias to 0 V and turn off.
    for ( int ch = 0; ch < 12; ++ch ) {
      DeviceWriteMask(lbneReg.bias_config[ch],0x00040FFF,0x0,board); // (formerly bias_dac_value)
    }
    DeviceWrite(lbneReg.bias_control,0x1,board); // write 0x1 here to load bias values (formerly bias_dac_loadvalues)
    err = DeviceDisconnect(commUSB,board);
  }

  // Write and close output file
  SSPTree->Write();

  for ( int board = 0; board < nDevices; ++board ) {
    for ( int chan = 0; chan < 12; ++chan ) {
      pulseAmp[board][chan]->Write();
      pulseInt[board][chan]->Write();
      AmpVInt[board][chan]->Write();
      WaveHistAll[board][chan]->Write();
    }
  }

  outFile->Close();

  // Done!
  std::cout << "Done!  Collected " << totArEvts << " waveforms." << std::endl;

  return 0;
}
