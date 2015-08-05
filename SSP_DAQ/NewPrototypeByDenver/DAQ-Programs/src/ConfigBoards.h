// ConfigBoards.h
// Configure CAEN Digitizer and ANL Digitizer for FreeRun mode.

#ifndef __CONFIGBOARDS_H__
#define __CONFIGBOARDS_H__

//////////
// Initialize the Argonne Digitizer
/////
int ConfigArgoBoard_SelfTrig( uint nSamples, uint preTrigSamples, uint leadEdgeThresh );
int ConfigArgoBoard_ExtTrig( uint nSamples, uint preTrigSamples );
int ConfigArgoBoard( uint nSamples, uint preTrigSamples, uint leadEdgeThresh, uint triggerMode );

#endif
