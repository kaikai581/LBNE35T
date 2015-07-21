#ifndef FLASH_H__
#define FLASH_H__

#define FLASH_PAGE_BYTES		0x100					// 256 bytes per page
#define FLASH_PAGE_WORDS		0x40					// 64 "words" per page (this is in no way physical)
#define FLASH_SECTOR_BYTES		FLASH_PAGE_BYTES * 16	// 16 pages per sector
#define FLASH_HALF_BLOCK_BYTES	FLASH_SECTOR_BYTES * 8	// 8 pages per (half) block
#define FLASH_BLOCK_BYTES		FLASH_SECTOR_BYTES * 16	// 16 pages per (full) block
#define FLASH_BLOCKS			256						// 256 blocks per flash chip

#include "DeviceInterface.h"

namespace SSPDAQ
{

class Flash
{
public:

  enum regionConstants {
    regionDSP		= 0,
    regionConfig	= 1,
    regionComm		= 2
  };
  
  struct Region {
    unsigned int		start;
    unsigned int		stop;
    unsigned int		blocks;
  };
  
  /// member functions
  /// a direct port of Mike's code
  int Erase		(SSPDAQ::DeviceInterface&, unsigned short);
  int Program	(SSPDAQ::DeviceInterface&, unsigned short, char[]);
  int Verify	(SSPDAQ::DeviceInterface&, unsigned short, char[]);
  /// helper functions
  int FileExists(char[], int*);

private:

  static Region flash_region[];
}; // class Flash

} // namespace

#endif
