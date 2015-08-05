// Configure and run Digitizer(s) in LED triggered mode

// C++ includes
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <vector>
#include <time.h>

// ANL includes
#include "LBNEWare.h"
#include "Device.h"

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

// TallBoDAQ includes
#include "ConfigBoards.h"

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
  std::cout << "LED triggered run for " << runTime << " seconds: " << outFileName << std::endl;

  // Create the ROOT Application (to draw canvases)
  TApplication *theApp = new TApplication("LEDRun",&nArgs,args);
  //gStyle->SetPalette(1);

  // Configure the Argonne Board
  int err(0);
  uint nSamples = 600; //1024; // max = 2046
  uint preTrigSamples = 50;
  err = ConfigArgoBoard_ExtTrig( nSamples, preTrigSamples );
  if ( err != 0 ) {
    std::cout << "Failed to configure ANL Digitizer." << std::endl;
    return err;
  }

  // Get ANL Digitizer configuration info
  uint ANL_nSamples(0); DeviceRead(lbneReg.readout_window[0],&ANL_nSamples);
  uint ANL_preTrigSize(0); DeviceRead(lbneReg.readout_pretrigger[0],&ANL_preTrigSize);
  uint ANL_eventSize = sizeof(Event_Header) + 2*nSamples; // in bytes
  uint m1Size(0); DeviceRead(lbneReg.m1_window[0],&m1Size);
  uint m2Size(0); DeviceRead(lbneReg.m2_window[0],&m2Size);
  uint pWindow(0); DeviceRead(lbneReg.p_window[0],&pWindow);
  uint kWindow(0); DeviceRead(lbneReg.k_window[0],&kWindow);
  uint iWindow(0); DeviceRead(lbneReg.i_window[0],&iWindow);
  uint dWindow(0); DeviceRead(lbneReg.d_window[0],&dWindow);

  // Output File
  TFile *outFile = new TFile(outFileName.c_str(),"RECREATE");
  TDirectory *waveDir = outFile->mkdir("waveforms");
  std::vector<TH1D*> waveformExamples;

  // Set up output ROOT tree for ANL Digitizer
  TTree *ANLTree = new TTree("ANLTree","ANLTree");
  Event_Packet ArPacket; Event ArEvent;
  ANLTree->Branch("channelID",&(ArEvent.channelID),"channelID/s");
  ANLTree->Branch("syncDelay",&(ArEvent.syncDelay),"syncDelay/i");
  ANLTree->Branch("syncCount",&(ArEvent.syncCount),"syncCount/i");
  ANLTree->Branch("timestamp",&(ArEvent.intTimestamp),"timestamp/l");
  ANLTree->Branch("peakSum",&(ArEvent.peakSum),"peakSum/I");
  ANLTree->Branch("peakTime",&(ArEvent.peakTime),"peakTime/C");
  ANLTree->Branch("prerise",&(ArEvent.prerise),"prerise/i");
  ANLTree->Branch("integratedSum",&(ArEvent.integratedSum),"integratedSum/i");
  ANLTree->Branch("baseline",&(ArEvent.baseline),"baseline/s");
  ANLTree->Branch("cfdPoint",&(ArEvent.cfdPoint),"cfdPoint[4]/S");
  ANLTree->Branch("nSamples",&(ArEvent.waveformWords),"nSamples/s");
  std::stringstream waveformDescr; waveformDescr << "waveform[" << nSamples << "]/s";
  ANLTree->Branch("waveform",&(ArEvent.waveform),waveformDescr.str().c_str());

  // Set up configuration ROOT tree
  TTree *configTree = new TTree("configTree","configTree");
  configTree->Branch("runTime",&runTime,"runTime/I");
  configTree->Branch("ANL_nSamples",&ANL_nSamples,"ANL_nSamples/i");
  configTree->Branch("ANL_preTrigSize",&ANL_preTrigSize,"ANL_preTrigSize/i");
  configTree->Branch("m1Size",&m1Size,"m1Size/i");
  configTree->Branch("m2Size",&m2Size,"m2Size/i");
  configTree->Branch("pWindow",&pWindow,"pWindow/i");
  configTree->Branch("kWindow",&kWindow,"kWindow/i");
  configTree->Branch("iWindow",&iWindow,"iWindow/i");
  configTree->Branch("dWindow",&dWindow,"dWindow/i");
  configTree->Fill();
  configTree->Write();

  // Monitoring histograms
  TH1D *ANL_pulseAmp[32];
  for ( int chan = 0; chan < 12; ++chan ) {
    std::stringstream histName; histName << "ANL_pulseAmp_CH" << chan;
    std::stringstream histTitle; histTitle << "Pulse Amplitudes, ANL Channel " << chan << ";ADC Counts";
    ANL_pulseAmp[chan] = new TH1D(histName.str().c_str(),histTitle.str().c_str(),3000,0,3000);
  }    

  std::cout << "Starting acquisition." << std::endl;
  // Enable boards
  err = DeviceTimeout(500);
  err = DeviceStart();

  // Loop for specified time
  int totArEvts(0);
  time_t tStart(0), tEnd(0);
  time(&tStart); time(&tEnd);
  while ( tEnd - tStart < runTime ) {

    /*** ANL Digitizer Readout ***/
    uint dataSize; err = DeviceQueueStatus(&dataSize); // How many full events have been collected?
    if ( err != 0 ) { std::cout << "Failed device queue status check." << std::endl; break; }
    uint ANL_nEvts = uint(dataSize) / ANL_eventSize;
    //if ( ANL_nEvts > 0 ) std::cout << "Received " << ANL_nEvts << " events on the Argonne Digitizer (" << dataSize << " bytes)." << std::endl;
    for ( uint evt = 0; evt < ANL_nEvts; ++evt ) {
      totArEvts++;
      uint dataReceived = 0;
      err = DeviceReceive(&ArPacket,&dataReceived);
      if ( err != 0 ) { std::cout << "Failed device receive." << std::endl; break; }
      err = LBNE_EventUnpack(&ArPacket,&ArEvent);
      if ( err != 0 ) {	std::cout << "Failed to unpack data." << std::endl; break; }
      ANLTree->Fill();
      ANL_pulseAmp[ArEvent.channelID]->Fill(ArEvent.prerise/double(kWindow)-ArEvent.peakSum/double(m1Size));
      /*
      std::stringstream waveHistName; waveHistName << "waveform_" << totArEvts;
      TH1D *hist = new TH1D(waveHistName.str().c_str(),waveHistName.str().c_str(),nSamples,0,nSamples);
      for ( uint samp = 0; samp < nSamples; ++samp ) {
	hist->SetBinContent(samp+1,ArEvent.waveform[samp] & 0x3FFF); // 0x3FFF drops time marks on waveform, not necessary in next firmware upgrade
      }
      waveformExamples.push_back(hist);
      */
    }

    //std::cout << tEnd-tStart << std::endl;
    time(&tEnd);
  }

  //// Finalize

  // Finalize Argonne Digitizer
  err = DeviceStopReset();
  if ( err != 0 ) {
    std::cout << "Failed to stop and reset board 0. ErrorCode " << err << std::endl;
    return 1;
  }
  err = DeviceDisconnect(commUSB);

  // Write and close output file
  ANLTree->Write();

  for ( int chan = 0; chan < 12; ++chan ) ANL_pulseAmp[chan]->Write();

  waveDir->cd();
  for ( uint i = 0; i < waveformExamples.size(); ++i ) {
    waveformExamples[i]->Write();
  }
  outFile->cd();

  outFile->Close();

  // Done!
  std::cout << "Done!  Collected " << totArEvts/12 << " events" << std::endl;

  return 0;
}
