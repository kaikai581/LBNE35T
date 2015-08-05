// ConfigBoards-FreeRun
// Configure CAEN Digitizer and ANL Digitizer for FreeRun mode.

// C++ includes
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <vector>
#include <time.h>

// ANL includes
#include "LBNEWare.h"
#include "Device.h"

//////////
// Initialize the Argonne Digitizer
/////
  /* Trigger/Waveform Polarity Modes
   * Positive Waveform Polarity
   *  Trigger:  external = 0x10F06201, self (negative) = 0x90F00801, self (positive) = 0x90F00401
   * Negative Waveform Polarity
   *  Trigger:  external = 0x00F06001, self (negative) = 0x80F00801, self (positive) = 0x80F00401
   */
int ConfigArgoBoard( uint nSamples, uint preTrigSamples, uint leadEdgeThresh, uint controlMode ) {

  std::cout << "Configuring Argonne Board." << std::endl;

  // Run Parameters
  uint m1Window = 20;
  uint m2Window = 10;
  uint pWindow = 2;
  uint kWindow = 100;
  uint iWindow = std::min(int(nSamples)-int(preTrigSamples),1023);

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
  //std::cout << "Discovered Argonne Board." << std::endl;

  // Connect to Argonne Board
  errorCode = DeviceConnect(commUSB,0, NULL, NULL);
  if ( errorCode != 0 ) {
    std::cout << "Failed to connect to board 0. ErrorCode " << errorCode << std::endl;
    return 1;
  }

  // Reset Argonne Board
  errorCode = DeviceStopReset(0);
  if ( errorCode != 0 ) {
    std::cout << "Failed to reset board 0. ErrorCode " << errorCode << std::endl;
    return 1;
  }

  // Initialize Argonne Board
  //std::cout << "Setting up board configuration." << std::endl;
  DeviceSetup(0); // Load defaults (defined in Device.c)
  // Override default run parameters with our configuration
  for ( int ch = 0; ch < 12; ++ch ) {
    DeviceWrite(lbneReg.readout_window[ch],nSamples, 0);
    DeviceWrite(lbneReg.readout_pretrigger[ch],preTrigSamples, 0);
    DeviceWrite(lbneReg.channel_control[ch],controlMode, 0);
    DeviceWrite(lbneReg.led_threshold[ch],leadEdgeThresh, 0);
    DeviceWrite(lbneReg.m1_window[ch],m1Window, 0);
    DeviceWrite(lbneReg.m2_window[ch],m2Window, 0);
    DeviceWrite(lbneReg.p_window[ch],pWindow, 0);
    DeviceWrite(lbneReg.i2_window[ch],kWindow, 0);
    DeviceWrite(lbneReg.i1_window[ch],iWindow, 0);
  }

  // Load channel delays that were just set
  DeviceWrite(lbneReg.channel_pulsed_control,1, 0);

  // Done!
  //std::cout << "Board configured and ready to run." << std::endl;

  return errorCode;
}
int ConfigArgoBoard_SelfTrig( uint nSamples, uint preTrigSamples, uint leadEdgeThresh ) {
  return ConfigArgoBoard(nSamples,preTrigSamples,leadEdgeThresh,0x80F00801);
}
int ConfigArgoBoard_ExtTrig( uint nSamples, uint preTrigSamples ) {
  return ConfigArgoBoard(nSamples,preTrigSamples,10,0x00F06001);
}
