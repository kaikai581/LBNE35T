#include <cstdio>
#include "Flash.h"

// O_BINARY is a windows thing. Linux doesn't distinguish O_BINARY and O_TEXT
#define O_BINARY 0

SSPDAQ::Flash::Region SSPDAQ::Flash::flash_region[] = {
	{0x10000000, 0x11000000, 256},	// regionDSP		0
	{0x20000000, 0x21000000, 256},	// regionConfig		1
	{0x30000000, 0x31000000, 256},	// regionComm		2
};

int SSPDAQ::Flash::Erase(SSPDAQ::DeviceInterface& dev, unsigned short region)
{
  char	status[256];	// Status text.
	
	// Initialize the address to the start of the 
	// first block in the region to be erased.
	unsigned int	address = flash_region[region].start;
	
	// Look up the number of blocks to erase. 
	// All regions are the physically the same size, but the
	// the number of block that will be erased can be adjusted
	// in the flash_region constant.
	unsigned short	blocks = flash_region[region].blocks;
	int		i = 0;									
													
	// Erase the entire region, one block at at time.
	for (i = 0; i < blocks; i++) {
		// Update the GUI status message.
		sprintf(status, "Erasing Block at 0x%8X", address);
		// < LabWindow GUI function calls removed >
		
		// Erase the next block.
		dev.EraseFirmwareBlock(address);
		
		// Increment to the starting address of the next block. 
		address += FLASH_BLOCK_BYTES;
	}
	
	// Update the GUI
	sprintf(status, "Erase Complete");
	// < LabWindow GUI function calls removed >
	
	return 0;
}

int SSPDAQ::Flash::Program(SSPDAQ::DeviceInterface& dev, unsigned short region, char path[256])
{
  char status[256];	// Status text.
	unsigned int address = 0;
	unsigned int addressStop = 0;
	unsigned int blocks = 0;
	unsigned short pages = 0;
	int j = 0;
	int file = 0;
	int fileEnd = 0;
	int fileType = 0;
	char* fileExtension;
	int bytesFile = 0;
	int bytesRead = 0;
	int bytesRegion = 0;
	unsigned char data[FLASH_PAGE_BYTES];

	// Initialize the address start and stop points, for the selected region.
	address		= flash_region[region].start;
	addressStop	= flash_region[region].stop;
	blocks		= flash_region[region].blocks;
	bytesRegion	= blocks * FLASH_BLOCK_BYTES;
	
	// Perform some checks on the provided file, before attempting to load it 
	// into the flash memory.
	file = FileExists(path, &bytesFile);
	if (file < 0) {
		// ERROR: The FileExists function threw an error.
		sprintf(status, "Error Checking For File");
		// < LabWindow GUI function calls removed >
		return -1;
	}
	else if (file == 0) {	
		// ERROR: File does not exist.
		sprintf(status, "File Does Not Exist");
		// < LabWindow GUI function calls removed >
		return -1;
	}
	else if (bytesFile > bytesRegion) {	
		// ERROR: The file is larger than the selected flash region.
		sprintf(status, "File Larger Than Selected Region");
		// < LabWindow GUI function calls removed >
		return -1;
	}
	else { // If no errors, open the file in a read-only binary mode.
		file = open(path, O_RDONLY | O_BINARY);
		if (file == -1) {	
			// There was an error opening the file.
			sprintf(status, "Error Opening File");
			// < LabWindow GUI function calls removed >
			return -1;
		}
	}
	
	fileExtension = strrchr(path,'.');	// Find last '.' in path
	if (fileExtension == NULL) {
		fileType = -1;
	} else {
		fileType = strcmp(fileExtension,".bin");	// Compare against ".bin"
	}
	
	if (fileType == 0) {
		// Flash Programming File had ".bin" extension
		// Program directly to flash region
		do {
			// Update the GUI
			sprintf(status, "Programming Page at 0x%08X", address);
			// < LabWindow GUI function calls removed >

			// Attempt to read a full page from the file
			bytesRead = read(file, data, FLASH_PAGE_BYTES);
			if (bytesRead != FLASH_PAGE_BYTES) {
				// Fill any remaining bytes in page with ones
				for (j = bytesRead; j < FLASH_PAGE_BYTES; j++) {
					data[j] = 0xFF;
				}
				fileEnd = 1;
			}
			// edited 
			dev.SetFirmwareArray(address, FLASH_PAGE_WORDS, (unsigned int*)(data));
			address += FLASH_PAGE_BYTES;
			pages++;
		}
		while ((address < addressStop) && (fileEnd == 0));
	} else {
		// Flash Programming File did NOT have ".bin" extension
		sprintf(status, "ERROR: Please select a *.bin file for programming");
		// < LabWindow GUI function calls removed >
	}

	close(file);
	// Update the GUI
	sprintf(status, "Programming Complete, %d Pages", pages);
	// < LabWindow GUI function calls removed >

	return 0;
}

int SSPDAQ::Flash::Verify(SSPDAQ::DeviceInterface& dev, unsigned short region, char path[256])
{
  char status[256];	// Status text.
	unsigned int address = 0;
	unsigned int addressStop = 0;
	unsigned short blocks = 0;
	unsigned short pages = 0;
	unsigned int errors = 0;
	int j = 0;
	int file = 0;
	int fileEnd = 0;
	int bytesFile = 0;
	int bytesRead = 0;
	int bytesRegion = 0;
	unsigned char dataFile[FLASH_PAGE_BYTES]; 
	unsigned char dataFlash[FLASH_PAGE_BYTES]; 

	address		= flash_region[region].start;
	addressStop	= flash_region[region].stop;
	blocks		= flash_region[region].blocks;
	bytesRegion	= blocks * FLASH_BLOCK_BYTES;

	// Perform some checks on the provided file, before attempting to load it 
	// into the flash memory.
	file = FileExists(path, &bytesFile);
	if (file < 0) {
		// ERROR: The FileExists function threw an error.
		sprintf(status, "Error Checking For File");
		// < LabWindow GUI function calls removed >
		return -1;
	}
	else if (file == 0) {	
		// ERROR: File does not exist.
		sprintf(status, "File Does Not Exist");
		// < LabWindow GUI function calls removed >
		return -1;
	}
	else if (bytesFile > bytesRegion) {	
		// ERROR: The file is larger than the selected flash region.
		sprintf(status, "File Larger Than Selected Region");
		// < LabWindow GUI function calls removed >
		return -1;
	}
	else { // If no errors, open the file in a read-only binary mode.
		file = open(path, O_RDONLY | O_BINARY);
		if (file == -1) {	
			// There was an error opening the file.
			sprintf(status, "Error Opening File");
			// < LabWindow GUI function calls removed >
			return -1;
		}
	}
	
	do {
		sprintf(status, "Verifying Page at 0x%08X, %d Errors", address, errors);
		// < LabWindow GUI function calls removed >
		
		// Attempt to read a full page from the file           
		bytesRead = read(file, dataFile, FLASH_PAGE_BYTES);
		if (bytesRead != FLASH_PAGE_BYTES) {
			// Fill any remaining bytes in page with ones   
			for (j = bytesRead; j < FLASH_PAGE_BYTES; j++) {
				dataFile[j] = 0xFF;
			}
			fileEnd = 1;
		}
		
		dev.ReadRegisterArray(address, (unsigned int*)(dataFlash), FLASH_PAGE_WORDS);

		// Compare against file and increment error counter
		for (j = 0; j < FLASH_PAGE_BYTES; j++) {
			if (dataFile[j] != dataFlash[j]) {
				errors++;
			}
		}
		
		address += FLASH_PAGE_BYTES;
		pages++;
	}
	while ((address < addressStop) && (fileEnd == 0));

	close(file);
	sprintf(status, "Verifying Complete, %d Pages, %d Errors\n", pages, errors);
	// < LabWindow GUI function calls removed >

	return 0;
}

int SSPDAQ::Flash::FileExists(char path[256], int* bytesFile)
{
  struct stat fileInfo;
  int res = stat(path, &fileInfo);
  *bytesFile = fileInfo.st_size;
  
  return res;
}
