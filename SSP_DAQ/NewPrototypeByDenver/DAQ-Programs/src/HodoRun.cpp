// Configure and run Digitizer(s) in hodoscope triggered mode

// C++ includes
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <vector>
#include <time.h>
#include <sys/time.h>

// SSP includes
#include "LBNEWare.h"
#include "Device.h"
#include "ftd2xx.h"

// CREST includes
#include "Overlord.h"
#include "HVController.h"
int VERBOSE = 0;
struct hodoHit {
  int cbus;
  int stac;
  int pmt;
  int adc0;
  int adc1;
  int tdc;
  int tick;
  int deadtime;
};

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
#include <TBox.h>
#include <TLine.h>
#include <TColor.h>

// Main Program
int main( int nArgs, char **args ) {

  bool drawHits(false);

  // Read command line parameters
  if ( nArgs < 3 ) {
    std::cout << "Please specify output filename and run time (seconds)." << std::endl;
    return 1;
  }
  std::string outFileName(args[1]);
  int runTime(30);
  { std::stringstream tmp; tmp << std::string(args[2]); tmp >> runTime; }
  std::cout << "Hodoscope run for " << runTime << " seconds: " << outFileName << std::endl;

  // Create the ROOT Application (to draw canvases)
  TApplication *theApp = new TApplication("HodoRun",&nArgs,args);
  //gStyle->SetPalette(1);

  // Configuration for the SSP
  int err(0);
  int totEvts(0);
  uint nSamples = 1950; // max = 2046
  uint preTrigSamples = 300;
  uint leadEdgeThresh = 0;
  /* Trigger/Waveform Polarity Modes
   * Positive Waveform Polarity
   *  Trigger:  external = 0x10F06201, self (negative) = 0x90F00801, self (positive) = 0x90F00401
   * Negative Waveform Polarity
   *  Trigger:  external = 0x00F06001, self (negative) = 0x80F00801, self (positive) = 0x80F00401
   */
  uint controlMode = 0x10F06001;
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

  // Locate SSP boards
  uint nDevices(0);
  err = DeviceDiscover(commUSB,&nDevices);
  if ( nDevices == 0 || nDevices > 2 ) {
    std::cout << "DeviceDiscover reports " << nDevices << " devices are connected. Expected 1 or 2, so quitting." << std::endl;
    return 1;
  }
  std::cout << "Discovered SSP " << nDevices << " board(s)." << std::endl;

  // Get SSP serial numbers
  std::string SerNum[nDevices];
  uint ModNum[nDevices];
  for ( uint board = 0; board < nDevices; ++board ) { 
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

  // Configure boards
  for ( uint brd = 0; brd < nDevices; ++brd ) {
    err = DeviceConnect(commUSB,brd,NULL,NULL);
    DeviceWrite(lbneReg.eventDataInterfaceSelect, 0x0, brd); // use USB interface
    err = DeviceStopReset(brd);

    std::cout << std::endl;
    uint modID(0);
    uint brdID(0);

    if ( err != 0 ) {
      std::cout << "Failed to connect to board " << brd << ". ErrorCode " << err << std::endl;
      return 1;
    }
    std::cout << "Connected to board " << brd << "." << std::endl;
    DeviceSetup(brd); // Load defaults (defined in Device.c)
    DeviceWrite(lbneReg.module_id,ModNum[brd],brd);

    std::cout << "Loading configuration parameters for board " << brd << "." << std::endl;
    for ( int ch = 0; ch < 12; ++ch ) {
      DeviceWrite(lbneReg.readout_window[ch],nSamples,brd);
      DeviceWrite(lbneReg.readout_pretrigger[ch],preTrigSamples,brd);
      DeviceWrite(lbneReg.channel_control[ch],controlMode,brd);
      DeviceWrite(lbneReg.led_threshold[ch],leadEdgeThresh,brd);
      DeviceWrite(lbneReg.m1_window[ch],m1Window,brd);
      DeviceWrite(lbneReg.m2_window[ch],m2Window,brd);
      DeviceWrite(lbneReg.p_window[ch],pWindow,brd);
      DeviceWrite(lbneReg.i2_window[ch],kWindow,brd);
      DeviceWrite(lbneReg.i1_window[ch],iWindow,brd);
      DeviceWriteMask(lbneReg.bias_config[ch],0x00040FFF,0x00040DA4,brd); // value & enable  D1A -> 24.5 V on output
    }

    // Load channel delays that were just set
    DeviceWrite(lbneReg.channel_pulsed_control,1,brd);

    DeviceRead(lbneReg.module_id,&modID,brd);
    DeviceRead(lbneReg.board_id,&brdID,brd);
    std::cout << "Configured Board " << brdID << " / Module " << modID << std::endl;
    //uint ctrlStat(0);
    //DeviceRead(lbneReg.control_status[0],&ctrlStat,brd);
    //std::cout << "Control Status: " << ctrlStat << std::endl;
    std::cout << std::endl;
  }

  if ( err != 0 ) {
    std::cout << "Failed to configure SSP(s)." << std::endl;
    return err;
  }
  std::cout << "Collecting " << nSamples/150. << " us waveforms with " << preTrigSamples/150. << " us pre-trigger." << std::endl;

  // Hodoscope interface
  int cbus[128];
  int stac[128];
  int pmt[128];
  int adc0[128];
  int adc1[128];
  int tdc[128];
  int tick[128];
  int deadtime[128];
  TTree *HodoTree;
  int _CBUS, _STAC, _PMT, _ADC0, _ADC1, _TDC, _TICK, _DEADTIME;
  HodoTree = new TTree("HodoTree","HodoTree");
  HodoTree->Branch("cbus",&_CBUS,"cbus/I");
  HodoTree->Branch("stac",&_STAC,"stac/I");
  HodoTree->Branch("pmt", &_PMT, "pmt/I" );
  HodoTree->Branch("adc0",&_ADC0,"adc0/I");
  HodoTree->Branch("adc1",&_ADC1,"adc1/I");
  HodoTree->Branch("tdc", &_TDC, "tdc/I" );
  HodoTree->Branch("tick",&_TICK,"tick/I");
  HodoTree->Branch("deadtime",&_DEADTIME,"deadtime/I");
  HodoTree->Branch("evtNum",&totEvts,"evtNum/I");
  // Debugging counters
  int HodoHits1[4][16];
  int HodoHits8[4][16];
  for ( int i = 0; i < 4; ++i ) {
    for ( int j = 0; j < 16; ++j ) {
      HodoHits1[i][j] = 0;
      HodoHits8[i][j] = 0;
    }
  }
  int TrackCounter[4][4];
  for ( int i = 0; i < 4; ++i ) {
    for ( int j = 0; j < 4; ++j ) {
      TrackCounter[i][j] = 0;
    }
  }

  // Hodoscope display
  TCanvas *HodoCan;
  if ( drawHits ) {
    HodoCan = new TCanvas("HodoCan","Hodscope Event View",300,600);
    HodoCan->SetWindowPosition(50,50);
    HodoCan->Divide(1,2,0.01,0.01);
  }
  TEllipse *HodoGridA[4][16]; // North
  TEllipse *HodoGridB[4][16]; // South
  int HodoColorA[4][16] = { {kYellow, kYellow-4, kYellow-7, kYellow-9, kYellow, kYellow-4, kYellow-7, kYellow-9, kYellow+1, kYellow-4, kYellow-7, kYellow-9, kYellow, kYellow-4, kYellow-7, kYellow-9}, 
			    {kGreen,  kGreen-4,  kGreen-7,  kGreen-9,  kGreen,  kGreen-4,  kGreen-7,  kGreen-9,  kGreen+1,  kGreen-4,  kGreen-7,  kGreen-9,  kGreen,  kGreen-4,  kGreen-7,  kGreen-9},
			    {kCyan,   kCyan-4,   kCyan-7,   kCyan-9,   kCyan,   kCyan-4,   kCyan-7,   kCyan-9,   kCyan+1,   kCyan-4,   kCyan-7,   kCyan-9,   kCyan,   kCyan-4,   kCyan-7,   kCyan-9},
			    {kBlue,   kBlue-4,   kBlue-7,   kBlue-9,   kBlue,   kBlue-4,   kBlue-7,   kBlue-9,   kBlue+1,   kBlue-4,   kBlue-7,   kBlue-9,   kBlue,   kBlue-4,   kBlue-7,   kBlue-9}    };
  int HodoColorB[4][16] = { {kYellow, kYellow-4, kYellow-7, kYellow-9, kYellow, kYellow-4, kYellow-7, kYellow-9, kYellow+1, kYellow-4, kYellow-7, kYellow-9, kYellow, kYellow-4, kYellow-7, kYellow-9}, 
			    {kGreen,  kGreen-4,  kGreen-7,  kGreen-9,  kGreen,  kGreen-4,  kGreen-7,  kGreen-9,  kGreen+1,  kGreen-4,  kGreen-7,  kGreen-9,  kGreen,  kGreen-4,  kGreen-7,  kGreen-9},
			    {kCyan,   kCyan-4,   kCyan-7,   kCyan-9,   kCyan,   kCyan-4,   kCyan-7,   kCyan-9,   kCyan+1,   kCyan-4,   kCyan-7,   kCyan-9,   kCyan,   kCyan-4,   kCyan-7,   kCyan-9},
			    {kBlue,   kBlue-4,   kBlue-7,   kBlue-9,   kBlue,   kBlue-4,   kBlue-7,   kBlue-9,   kBlue+1,   kBlue-4,   kBlue-7,   kBlue-9,   kBlue,   kBlue-4,   kBlue-7,   kBlue-9}    };
  double chAstackOffsetX[4] = { 0.0, 0.5, 0.5, 0.0 };
  double chAstackOffsetY[4] = { 0.5, 0.0, 0.5, 0.0 };
  double chBstackOffsetX[4] = { 0.5, 0.0, 0.0, 0.5 };
  double chBstackOffsetY[4] = { 0.5, 0.5, 0.0, 0.0 };
  for ( int stac = 0; stac < 4; ++stac ) {
    for ( int PMT = 0; PMT < 16; ++PMT ) {
      int i = PMT % 4;
      int j = (PMT - i)/4;
      // Channel A (North)
      double yA = chAstackOffsetY[stac] + double(i)*(1./8.) + 1./16.;
      double xA = chAstackOffsetX[stac] + double(3-j)*(1./8)+1./16.;
      xA = 1. - xA;
      if ( drawHits ) HodoCan->cd(1);
      HodoGridA[stac][PMT] = new TEllipse(xA, yA, 0.05, 0.05);
      if ( drawHits ) HodoGridA[stac][PMT]->Draw();
      // Channel B (South)
      double yB = chBstackOffsetY[stac] + double(i)*(1./8.) + 1./16.;
      double xB = chBstackOffsetX[stac] + double(3-j)*(1./8)+1./16.;
      if ( drawHits ) HodoCan->cd(2);
      HodoGridB[stac][PMT] = new TEllipse(xB, yB, 0.05, 0.05);
      if ( drawHits ) HodoGridB[stac][PMT]->Draw();
    }
  }
  if ( drawHits ) HodoCan->Draw();

  TCanvas *PhotCan;
  TH2D* TallBoYZ;
  if ( drawHits ) {
    TallBoYZ = new TH2D("TallBoYZ","TallBo, Side View;z [in];y (vertical) [in]",40,-20,20,80,0,80);
    TallBoYZ->SetStats(false);
    TallBoYZ->GetXaxis()->SetTitleOffset(1.2);
    TallBoYZ->GetYaxis()->SetTitleOffset(1.3);
    PhotCan = new TCanvas("PhotCan","Photon Detector Event View",600,800);
    PhotCan->SetWindowPosition(400,50);
    PhotCan->cd();
    TallBoYZ->Draw();
  }
  TLine *dewarLeft  = new TLine(-21.5/2,0,-21.5/2,80); dewarLeft->SetLineColor(4);  if ( drawHits ) dewarLeft->Draw();
  TLine *dewarRight = new TLine( 21.5/2,0, 21.5/2,80); dewarRight->SetLineColor(4); if ( drawHits ) dewarRight->Draw();
  TBox *PaddleArray[3];
  TBox *PaddleA = new TBox(-6.00-1.75,71.25,-6.00+1.75,11.25); PaddleArray[0] = PaddleA; PaddleA->SetLineColor(16); PaddleA->SetFillColor(29); if ( drawHits ) PaddleA->Draw();
  TBox *PaddleB = new TBox( 0.00-1.75,71.25, 0.00+1.75,11.25); PaddleArray[1] = PaddleB; PaddleB->SetLineColor(16); PaddleB->SetFillColor(29); if ( drawHits ) PaddleB->Draw();
  TBox *PaddleC = new TBox( 6.00-1.75,71.25, 6.00+1.75,11.25); PaddleArray[2] = PaddleC; PaddleC->SetLineColor(16); PaddleC->SetFillColor(29); if ( drawHits ) PaddleC->Draw();
  int PhotPalette[100];
  double PhotR[]    = {0.0, 1.0, 1.0 };
  double PhotG[]    = {0.0, 0.0, 1.0 };
  double PhotB[]    = {1.0, 0.0, 1.0 };
  double PhotStop[] = {0.0, 0.4, 1.0 };
  int PhotTab = TColor::CreateGradientColorTable(3, PhotStop, PhotR, PhotG, PhotB, 100);
  for ( int i = 0; i < 100; i++ ) PhotPalette[i] = PhotTab+i;
  if ( drawHits ) PhotCan->Draw();

  // Initialize the Hodoscope 
  Overlord *olord = new Overlord();
  //HVController *hv = new HVController(0);
  //hv->Initialize();

  // Output File
  TFile *outFile = new TFile(outFileName.c_str(),"RECREATE");
  TDirectory *waveDir = outFile->mkdir("waveforms");
  std::vector<TH1D*> waveformExamples;

  // Set up output ROOT tree for SSP Digitizer
  TTree *SSPTree = new TTree("SSPTree","SSPTree");
  Event_Packet ArPacket; Event ArEvent;
  uint eventSize = sizeof(Event_Header) + 2*nSamples; // in bytes
  SSPTree->Branch("evtNum",&totEvts,"evtNum/I");
  SSPTree->Branch("moduleID",&(ArEvent.moduleID),"moduleID/s");
  SSPTree->Branch("channelID",&(ArEvent.channelID),"channelID/s");
  SSPTree->Branch("syncDelay",&(ArEvent.syncDelay),"syncDelay/i");
  SSPTree->Branch("syncCount",&(ArEvent.syncCount),"syncCount/i");
  SSPTree->Branch("timestamp",&(ArEvent.intTimestamp),"timestamp/l");
  SSPTree->Branch("peakSum",&(ArEvent.peakSum),"peakSum/I");
  SSPTree->Branch("peakTime",&(ArEvent.peakTime),"peakTime/C");
  SSPTree->Branch("prerise",&(ArEvent.prerise),"prerise/i");
  SSPTree->Branch("integratedSum",&(ArEvent.integratedSum),"integratedSum/i");
  SSPTree->Branch("baseline",&(ArEvent.baseline),"baseline/s");
  SSPTree->Branch("cfdPoint",&(ArEvent.cfdPoint),"cfdPoint[4]/S");
  SSPTree->Branch("nSamples",&(ArEvent.waveformWords),"nSamples/s");
  std::stringstream waveformDescr; waveformDescr << "waveform[" << nSamples << "]/s";
  SSPTree->Branch("waveform",&(ArEvent.waveform),waveformDescr.str().c_str(),sizeof(ushort)*nSamples); // Don't forget to set the buffer size for this one!!!

  // Set up configuration ROOT tree
  TTree *configTree = new TTree("configTree","configTree");
  configTree->Branch("runTime",&runTime,"runTime/I");
  configTree->Branch("nSamples",&nSamples,"nSamples/i");
  configTree->Branch("preTrigSamples",&preTrigSamples,"preTrigSamples/i");
  configTree->Branch("m1Window",&m1Window,"m1Window/i");
  configTree->Branch("m2Window",&m2Window,"m2Window/i");
  configTree->Branch("pWindow",&pWindow,"pWindow/i");
  configTree->Branch("kWindow",&kWindow,"kWindow/i");
  configTree->Branch("iWindow",&iWindow,"iWindow/i");
  configTree->Branch("dWindow",&dWindow,"dWindow/i");
  configTree->Branch("biasSetting",&biasSetting,"biasSetting/i");
  configTree->Fill();
  configTree->Write();

  // Supply bias
  for ( uint brd = 0; brd < nDevices; ++brd ) {
    // Activate internal bias
    DeviceWrite(lbneReg.bias_control,0x1,brd); // write 0x1 here to load bias values
  }
  sleep(2);

  std::cout << "Starting acquisition." << std::endl;

  // Enable HodoDAQ
  int nConsecBadEvts(0);
  olord->EnableHK();
  olord->CBCReset();
  olord->EnableDAQ();

  // Enable boards
  for ( uint brd = 0; brd < nDevices; ++brd ) {
    err = DeviceTimeout(50,brd);
    err = DeviceStart(brd);
  }

  // Loop for specified time
  timespec startTime, prevTime, currTime, lastEvtTime;
  clock_gettime(CLOCK_REALTIME,&startTime);
  prevTime = startTime;
  currTime = startTime;
  lastEvtTime = startTime;
  while ( currTime.tv_sec-startTime.tv_sec < runTime ) {
    clock_gettime(CLOCK_REALTIME,&currTime );

    if ( currTime.tv_sec - lastEvtTime.tv_sec > 300 ) {
      std::cout << "Timeout. No triggers for the past five minutes. Resetting hodoscope DAQ and trying again." << std::endl;
      olord->EnableHK();
      olord->CBCReset();
      olord->EnableDAQ();   
      lastEvtTime = currTime;
      continue;
    }

    if ( olord->DataAvail() ) { // check for data

      // Check for hodoscope data
      olord->ReadBuffer(true);
      olord->ProcessEventBuffer();
      int nHits = olord->getData(cbus,stac,pmt,adc0,adc1,tdc,tick,deadtime);

      if ( nHits == 0 ) continue;
      std::cout << std::dec << "Received " << nHits << " hodoscope hits. Processing event." << std::endl;
      bool recordEvent(true);
      uint nEvts[2] = {0,0};

      if ( drawHits ) {
	/*** Clear Hodoscope display ***/
	for ( int i = 0; i < 4; ++i ) {
	  for ( int j = 0; j < 16; ++j ) {
	    HodoGridA[i][j]->SetFillColor(HodoColorA[i][j]);
	    HodoGridB[i][j]->SetFillColor(HodoColorB[i][j]);
	  }
	}
	/*** Clear Photon Detector display ***/
	for ( int i = 0; i < 3; ++i ) { PaddleArray[i]->SetFillColor(29); }
      }

      /*** SSP Digitizer Readout ***/
      Event ArEventBuffer[2][12];
      bool HaveEvent[2][12] = { { false, false, false, false, false, false, false, false, false, false, false, false },
				{ false, false, false, false, false, false, false, false, false, false, false, false } };
      for ( int brd = 0; brd < int(nDevices); ++brd ) {
	//err = DeviceConnect(commUSB,brd);

	uint dataSize; err = DeviceQueueStatus(&dataSize,brd); // How many full events have been collected?
	if ( err != 0 ) { std::cout << "Failed device queue status check." << std::endl; break; }
	nEvts[brd] = uint(dataSize) / eventSize;

	if ( dataSize > 0 && nEvts[brd] < 12 ) {
	  std::cout << " Accumulating remaining waveforms..." << std::endl;
	  clock_gettime(CLOCK_REALTIME,&currTime);
	  prevTime = currTime;
	  while ( nEvts[brd] < 12 && (currTime.tv_nsec-prevTime.tv_nsec)/1000000. < 10./*millisec*/ ) {
	    if ( currTime.tv_nsec < prevTime.tv_nsec ) prevTime = currTime; // rollover protection
	    clock_gettime(CLOCK_REALTIME,&currTime);
	    err = DeviceQueueStatus(&dataSize,brd); // How many full events have been collected?
	    if ( err != 0 ) { std::cout << "Failed device queue status check in accumulation loop." << std::endl; break; }
	    nEvts[brd] = uint(dataSize) / eventSize;
	  }
	  std::cout << " Now have " << dataSize << " bytes of data on SSP " << brd << " (" << nEvts[brd] << " waveforms) after a " << (currTime.tv_nsec-prevTime.tv_nsec)/1000000. << " ms accumulation time." << std::endl;
	}
	err = DeviceQueueStatus(&dataSize,brd);
	std::cout << "Found " << dataSize << " bytes of data on SSP " << brd <<  " (" << nEvts[brd] << " waveforms)." << std::endl;

	for ( int evt = 0; evt < std::min(int(nEvts[brd]),12); ++evt ) {
	  uint dataReceived = 0;
	  err = DeviceReceive(&ArPacket,&dataReceived,brd);
	  if ( err != 0 ) { std::cout << "Failed device receive on board " << brd << " waveform " << evt << std::endl; continue; }
	  err = LBNE_EventUnpack(&ArPacket,&ArEvent);
	  if ( err != 0 ) { std::cout << "Failed to unpack data." << std::endl; continue; }
	  if ( ArEvent.channelID < 0 || ArEvent.channelID > 11 ) {
	    std::cout << "Caution! Unexpected channel number: " << ArEvent.moduleID << "/" << ArEvent.channelID << std::endl;
	  } else {
	    //std::cout << "Buffering waveform " << evt << ": module " << ArEvent.moduleID << " / channel " << ArEvent.channelID << std::endl;
	    ArEventBuffer[brd][ArEvent.channelID] = ArEvent;
	    HaveEvent[brd][ArEvent.channelID] = true;
	  }
	}

	err = DevicePurgeData(brd); // Clear extraneous data... Seems to be a problem for CSU board.
      }

      for ( int brd = 0; brd < int(nDevices); ++brd ) {
	for ( int ch = 0; ch < 12; ++ch ) {
	  recordEvent &= HaveEvent[brd][ch];
	}
      }

      if ( !recordEvent ) {
	nConsecBadEvts++;
	std::cout << "Warning: Event will not be recorded." << std::endl;
	std::cout << "************************" << std::endl;
	if ( nConsecBadEvts >= 10 ) {
	  olord->EnableHK();
	  olord->CBCReset();
	  olord->EnableDAQ();
	}
	continue;
      }
      totEvts++;
      nConsecBadEvts = 0;
      lastEvtTime = currTime;

      /*** Record SSP Data ***/
      double ampSum[3] = {0,0,0};
      for ( int brd = 0; brd < int(nDevices); ++brd ) {
	for ( int ch = 0; ch < 12; ++ch ) {
	  ArEvent = ArEventBuffer[brd][ch];
	  SSPTree->Fill();
	  if ( totEvts <= 100 ) { // Save the first 100 waveforms as examples
	    std::stringstream waveHistName; waveHistName << "waveform_" << totEvts << "_" << ArEvent.moduleID << "_" << ArEvent.channelID;
	    std::stringstream waveHistTitle; waveHistTitle << "SSP " << ArEvent.moduleID << " Channel " << ArEvent.moduleID << "/" << ArEvent.channelID;
	    TH1D *hist = new TH1D(waveHistName.str().c_str(),waveHistTitle.str().c_str(),nSamples,0,nSamples);
	    for ( uint samp = 0; samp < nSamples; ++samp ) {
	      hist->SetBinContent(samp+1,ArEvent.waveform[samp]);
	      hist->SetStats(false);
	    }
	    waveformExamples.push_back(hist);
	  }
	  if ( ArEvent.moduleID ==  2 && ArEvent.channelID  < 6 ) ampSum[0] += std::max(ArEvent.prerise/double(kWindow)-ArEvent.peakSum/double(m1Window),0.);
	  if ( ArEvent.moduleID ==  2 && ArEvent.channelID >= 6 ) ampSum[1] += std::max(ArEvent.prerise/double(kWindow)-ArEvent.peakSum/double(m1Window),0.);
	  if ( ArEvent.moduleID == 18                           ) ampSum[2] += std::max(ArEvent.prerise/double(kWindow)-ArEvent.peakSum/double(m1Window),0.);
	}
      }
      if ( drawHits ) {
	//ampSum[0] /= 6; ampSum[1] /= 2; ampSum[2] /= 9;
	int colorVal[3] = { PhotPalette[std::min(int(ampSum[0]/24),99)], PhotPalette[std::min(int(ampSum[1]/24),99)], PhotPalette[std::min(int(ampSum[2]/24),99)] };
	PaddleArray[0]->SetFillColor(colorVal[0]); PaddleArray[1]->SetFillColor(colorVal[1]); PaddleArray[2]->SetFillColor(colorVal[2]);
	PhotCan->cd()->Modified(); PhotCan->cd()->Update();
      }

      /*** Record Hodoscope Data ***/
      std::cout << "Recording " << nHits << " hodoscope hits" << std::endl;
      int trkStac1 = 0;
      int trkStac8 = 0;
      bool cbusHit1 = false;
      bool cbusHit8 = false;
      for ( int i = 0; i < nHits; ++i ) {
	//std::cout << cbus[i] << " " << stac[i] << " " << pmt[i] << " " << 2148-adc0[i] << " " << 2148-adc1[i] << " " << tdc[i] << " " << tick[i] << std::endl;
	//std::cout << i << ":  " << tdc[i] << " " << tick[i] << std::endl;
	hodoHit thisHit;
	thisHit.cbus = cbus[i]; _CBUS = cbus[i];
	thisHit.stac = stac[i]; _STAC = stac[i];
	thisHit.pmt  = pmt[i];  _PMT  =  pmt[i];
	thisHit.adc0 = adc0[i]; _ADC0 = adc0[i];
	thisHit.adc1 = adc1[i]; _ADC1 = adc1[i];
	thisHit.tdc  = tdc[i];  _TDC  =  tdc[i];
	thisHit.tick = tick[i]; _TICK = tick[i];
	thisHit.deadtime = deadtime[i]; _DEADTIME = deadtime[i];
	if ( thisHit.stac-1 > 3 || thisHit.pmt > 15 ) continue;
	HodoTree->Fill();
	// Record this hit in the debugging counters
	if ( cbus[i] == 1 ) {
	  cbusHit1 = true;
	  HodoHits1[stac[i]-1][pmt[i]]++;
	  trkStac1 = stac[i];
	}
	if ( cbus[i] == 8 ) {
	  cbusHit8 = true;
	  HodoHits8[stac[i]-1][pmt[i]]++;
	  trkStac8 = stac[i];
	}
	// Update display
	if ( thisHit.cbus == 1 ) HodoGridA[ thisHit.stac-1 ][ thisHit.pmt ]->SetFillColor(2);
	if ( thisHit.cbus == 8 ) HodoGridB[ thisHit.stac-1 ][ thisHit.pmt ]->SetFillColor(2);
	// Report track info for status check
	if ( nHits == 2 ) {
	  std::cout << " Track hit: " << thisHit.cbus << " " << thisHit.stac << " " << thisHit.pmt << std::endl;
	}
      }
      if ( nHits == 2 && cbusHit1 && cbusHit8 ) TrackCounter[trkStac1-1][trkStac8-1]++;
      if ( drawHits ) { HodoCan->cd(1)->Modified(); HodoCan->cd(1)->Update(); }
      if ( drawHits ) { HodoCan->cd(2)->Modified(); HodoCan->cd(2)->Update(); }

      std::cout << "Recorded event " << totEvts << std::endl;
      std::cout << "************************" << std::endl;
    }

    usleep(1000); // Slow down the event loop since the data rate is so slow.
    prevTime = currTime;
  }
  std::cout << "Finished acquisition." << std::endl;

  TH2I *HitMapBus1 = new TH2I("HitMapBus1","Hit Map, Bus 1;PMT;Stack",16,0,16,4,1,5);
  TH2I *HitMapBus8 = new TH2I("HitMapBus8","Hit Map, Bus 8;PMT;Stack",16,0,16,4,1,5);
  for ( int i = 0; i < 4; ++i ) {
    for ( int j = 0; j < 16; ++j ) {
      //std::cout << "Bus 1  stack " << i+1 << " pmt " << j << " : " << HodoHits1[i][j] << " hits" << std::endl;
      //std::cout << "Bus 8  stack " << i+1 << " pmt " << j << " : " << HodoHits8[i][j] << " hits" << std::endl;
      HitMapBus1->Fill(j,i+1,HodoHits1[i][j]);
      HitMapBus8->Fill(j,i+1,HodoHits8[i][j]);
    }
  }

  //// Finalize

  // Finalize SSP Digitizer
  for ( uint brd = 0; brd < nDevices; ++brd ) {
    std::cout << "Finalizing board " << brd << std::endl;
    err = DeviceStopReset(brd);

    // Set internal bias to 0 V and turn off.
    for ( int ch = 0; ch < 12; ++ch ) {
      DeviceWriteMask(lbneReg.bias_config[ch],0x00040FFF,0x0,brd);
    }
    DeviceWrite(lbneReg.bias_control,0x1,brd); // write 0x1 here to load bias values

    err = DeviceDisconnect(commUSB,brd);
  }

  // Write and close output file
  HodoTree->Write();
  SSPTree->Write();
  HitMapBus1->Write();
  HitMapBus8->Write();

  waveDir->cd();
  for ( uint i = 0; i < waveformExamples.size(); ++i ) {
    waveformExamples[i]->Write();
  }
  outFile->cd();

  outFile->Close();

  for ( int i = 0; i < 4; ++i ) {
    for ( int j = 0; j < 4; ++j ) {
      std::cout << TrackCounter[i][j] << " tracks from bus 1 stac " << i+1 << " to bus 8 stac " << j+1 << std::endl;
    }
  }

  // Done!
  std::cout << "Done!  Collected " << totEvts << " events." << std::endl;

  return 0;
}
