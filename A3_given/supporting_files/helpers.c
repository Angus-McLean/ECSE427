
#include "sfs_api.h"
#include "bitmap.h"
// #include "helpers.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <fuse.h>
#include <strings.h>
#include "disk_emu.h"
#include "globals.c"
#include "helpers.h"

/////// Helper Functions ///////

int checkNameIsValid(const char* name) {
	int length = strlen(name);
	if (length > MAX_FILE_NAME)
		return -1;
	char copy[length+1];
	memset(copy, '\0', sizeof(copy));
	strcpy(copy, name); // because strtok modifies the string

	char* token;
	const char s[2] = ".";
	token = strtok(copy, s); // token = "([]).[]"
	token = strtok(NULL, s); // token = "[].([])"
	if (strlen(token) > MAX_EXTENSION_NAME)
		return -1;
	return 0;
}

void writeBlocksToBuf(void *data, int dataSize) {
  int blockedSize = (dataSize/BLOCK_SIZE + (dataSize%BLOCK_SIZE > 0)) * BLOCK_SIZE;
  buffer = (void*) malloc(blockedSize);
	memset(buffer, 0, blockedSize);
	memcpy(buffer, data, dataSize);
}

void readBlocksToBuf(int start_address, int nblocks) {
  buffer = (void*) malloc(nblocks*BLOCK_SIZE);
  memset(buffer, 0, nblocks*BLOCK_SIZE);
  read_blocks(start_address, nblocks, buffer);
}

int write_and_bitmap(int start_address, int nblocks, void *buffer){
  int blocksWritten = write_blocks(start_address, nblocks, buffer);
  for (int i=start_address; i<nblocks; i++)
    force_set_index(i);
  return blocksWritten;
}

int saveINodeTable(int updateBitMap) {
  writeBlocksToBuf(cachedINodeTable, (INODE_TABLE_LEN)*(sizeof(inode_t)));
  int blocksWritten;
  if(updateBitMap) {
    blocksWritten = write_and_bitmap(1, TOTAL_NUM_BLOCKS_INODETABLE, buffer);
  } else {
    write_blocks(1, TOTAL_NUM_BLOCKS_INODETABLE, buffer);
  }
	free(buffer);
	return blocksWritten;
}

int saveRootDirectory(int updateBitMap) {
  writeBlocksToBuf(cachedFiles, (NUM_FILES)*(sizeof(directory_entry)));
  int blocksWritten;
  if(updateBitMap) {
    blocksWritten = write_and_bitmap(START_ADDRESS_OF_DATABLOCKS, TOTAL_NUM_BLOCKS_ROOTDIR, buffer);
  } else {
    write_blocks(START_ADDRESS_OF_DATABLOCKS, TOTAL_NUM_BLOCKS_ROOTDIR, buffer);
  }
	free(buffer);
	return blocksWritten;
}

int saveFreeBitMap(int updateBitMap) {
  writeBlocksToBuf(free_bit_map, (SIZE)*(sizeof(uint8_t)));
  int blocksWritten;
  if(updateBitMap) {
    blocksWritten = write_and_bitmap(NUM_BLOCKS-1, 1, buffer);
  } else {
    write_blocks(NUM_BLOCKS-1, 1, buffer);
  }
	free(buffer);
	return blocksWritten;
}

int calcByteAddress(int blockNum, int offset) {
	return BLOCK_SIZE*blockNum+offset;
}
