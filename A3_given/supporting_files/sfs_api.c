
#include "sfs_api.h"
#include "bitmap.h"
// #include "helpers.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <fuse.h>
#include <strings.h>
#include "disk_emu.h"

/* macros */
#define FREE_BIT(_data, _which_bit) \
    _data = _data | (1 << _which_bit)

#define USE_BIT(_data, _which_bit) \
    _data = _data & ~(1 << _which_bit)


#include "globals.c"
#include "helpers.h"


int filesVisited = 0;
int totalFiles = 0;

/////// Provided SFS API Functions ///////
void mksfs(int fresh) {
  int i;
  if (fresh) {
	  init_fresh_disk(MCLEAN_ANGUS_DISK, BLOCK_SIZE, NUM_BLOCKS);

    // initialize the superblock
    superblock = (superblock_t) {0xACBD0005, BLOCK_SIZE, NUM_BLOCKS, INODE_TABLE_LEN, 0};
    buffer = (void*) malloc(BLOCK_SIZE);
    memset(buffer, 0, BLOCK_SIZE);
		memcpy(buffer, &superblock, sizeof(superblock_t));

    if (write_and_bitmap(0, 1, buffer) < 0) printf("Failed to write super block\n");
		free(buffer);

    // initialize inode table with empty inodes
    cachedINodeTable[0] = (inode_t) {777, 0, 0, 0, 0, {START_ADDRESS_OF_DATABLOCKS, START_ADDRESS_OF_DATABLOCKS+1, START_ADDRESS_OF_DATABLOCKS+2, -1, -1, -1, -1, -1, -1, -1, -1, -1}, -1};
    for (int i=1; i<INODE_TABLE_LEN; i++)
      cachedINodeTable[i] = (inode_t) {777, 0, 0, 0, -1, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, -1};
    if (saveINodeTable(1) < 0) printf("Failed to writed inode table\n");

    // init directory entries
		for (i=0; i<NUM_FILES; i++) {
			cachedFiles[i].num = -1;
			memset(cachedFiles[i].name, '\0', sizeof(cachedFiles[i].name));
		}
		if (saveRootDirectory(1) < 0) printf("Failed to write root directory\n");


    // writing freeBitMap to disk
		if (saveFreeBitMap(1) < 0) printf("Failed to write freeBitMap to disk\n");
	} else {
    init_disk(MCLEAN_ANGUS_DISK, BLOCK_SIZE, NUM_BLOCKS);

    // get freebitmap from disk and load to variables
    readBlocksToBuf(NUM_BLOCKS-1, 1);
    memcpy(free_bit_map, buffer, (SIZE)*(sizeof(uint8_t)));
		free(buffer);
		force_set_index(NUM_BLOCKS-1);


		// get superblock from disk and load to variables
    readBlocksToBuf(0, 1);
    memcpy(&superblock, buffer, sizeof(superblock_t));
		free(buffer);
		force_set_index(0);


		// get inode table from disk and load to variables
    readBlocksToBuf(1, TOTAL_NUM_BLOCKS_INODETABLE);
    memcpy(cachedINodeTable, buffer, INODE_TABLE_LEN*sizeof(inode_t));
		free(buffer);
		for (i=0; i<TOTAL_NUM_BLOCKS_INODETABLE; i++)
			force_set_index(i+1);


		// get root directory from disk and load to variables
    readBlocksToBuf(START_ADDRESS_OF_DATABLOCKS, TOTAL_NUM_BLOCKS_ROOTDIR);
		memcpy(cachedFiles, buffer, NUM_FILES*sizeof(directory_entry));
		free(buffer);
		for (i=0; i<TOTAL_NUM_BLOCKS_ROOTDIR; i++)
			force_set_index(i+START_ADDRESS_OF_DATABLOCKS);

  }
  // init file descriptors
  cachedFd[0] = (file_descriptor) {0, &cachedINodeTable[0], 0};
  for (i=1; i<INODE_TABLE_LEN; i++) {
    cachedFd[i] = (file_descriptor) {-1, NULL, 0};
  }
}


int sfs_getnextfilename(char *fname) {
	if (totalFiles == filesVisited) { // get back to beginning of the list
		filesVisited = 0;
		return 0;
	}

	// iterate through all files in root
	int fileIndex = 0;
	int i;
	for (i=0; i<NUM_FILES; i++){
		if (cachedFiles[i].num < 0) {
			if (fileIndex == filesVisited) {
				memcpy(fname, cachedFiles[i].name, sizeof(cachedFiles[i].name));
				break;
			}
			fileIndex++;
		}
	}
	filesVisited++;
	return 1;
}

int sfs_getfilesize(const char* path) {
  // TODO : validate names
  // if (checkNameIsValid(path) < 0)
	// 	return -1;

	for (int i=0; i<NUM_FILES; i++) {
		if (cachedFiles[i].num != -1 && !strcmp(cachedFiles[i].name, path))
			return cachedINodeTable[cachedFiles[i].num].size;
	}
	printf("File not found : %s\n", path);
	return -1;
}

int sfs_fopen(char *name) {
  // TODO : validate names
	// if (checkNameIsValid(name) < 0)
	// 	return -1;


	int iNodeInd = -1;
	int i;
  // iterate to find file
	for (i=0; i<NUM_FILES; i++) {
    // printf("Iterating %d, %s\n", i, cachedFiles[i].name);
		if (!strcmp(cachedFiles[i].name, name)) {
			iNodeInd = cachedFiles[i].num;
			break;
		}
	}

	if (iNodeInd > -1) {
    // file exists at
		for (i=1; i<INODE_TABLE_LEN; i++) {
			if (cachedFd[i].inodeIndex == iNodeInd) {
				return i;
			}
		}
	} else {
		int fileIndex = -1;
		for (i=0; i<NUM_FILES; i++) {
			if (cachedFiles[i].num < 0) {
				fileIndex = i;
				break;
			}
		}
		if (fileIndex == -1) {
			printf("Root directory full\n");
			return -1;
		}


		for (i=1; i<INODE_TABLE_LEN; i++) {
			if (cachedINodeTable[i].size == -1) {
				iNodeInd = i;
				break;
			}
		}
		if (iNodeInd < 0) {
			printf("INode not found\n");
			return -1;
		}

		strcpy(cachedFiles[fileIndex].name, name);
		cachedFiles[fileIndex].num = iNodeInd;
		cachedINodeTable[iNodeInd].size = 0;

		if (saveINodeTable(0) < 0)
			printf("Failure(s) writing iNode to disk");
		if (saveRootDirectory(0) < 0)
			printf("Failure(s) writing root directory to disk");
	}


	int fdIndex = -1;
	for (i=1; i<INODE_TABLE_LEN; i++) {
		if (cachedFd[i].inodeIndex == -1) {
			fdIndex = i;
			break;
		}
	}
	if (i == INODE_TABLE_LEN) {
		printf("Open file descriptor table full. Investigate\n");
		return -1;
	}

	int rwptr = cachedINodeTable[iNodeInd].size;
	cachedFd[fdIndex] = (file_descriptor) {iNodeInd, &cachedINodeTable[iNodeInd], rwptr};
	totalFiles++;

	return fdIndex;
}

int sfs_fclose(int fileID) {
	if (fileID <= 0 || fileID > NUM_FILES) { // fileID 0 is reserved for root
		printf("fileID %d is invalid\n", fileID);
		return -1;
	}

	// check if file already closed
	if (cachedFd[fileID].inodeIndex == -1) {
		printf("File is already closed\n");
		return -1;
	}

	cachedFd[fileID] = (file_descriptor) {-1, NULL, 0};
	totalFiles--;
	return 0;
}

int sfs_fread(int fileID, char *buf, int length) {
	if (length < 0) {
		printf("Length cannot be negative");
		return -1;
	}

	file_descriptor *tmpFd;
	tmpFd = &cachedFd[fileID];

	// check that file is open
	if ((*tmpFd).inodeIndex == -1) {
		printf("File is not open");
		return -1;
	}

	inode_t *activeINode;
	activeINode = (*tmpFd).inode;

	// calculate nb of blocks needed and index inside last block
	int rwptr = (*tmpFd).rwptr;
	int startBlockIndex = rwptr / BLOCK_SIZE;
	int startOffsetInBlock = rwptr % BLOCK_SIZE; // start writing here
	int endRwptr;
	if (rwptr+length > (*activeINode).size) {
		endRwptr = (*activeINode).size;
	} else {
		endRwptr = rwptr + length;
	}
	int endBlockIndex = endRwptr / BLOCK_SIZE;
	int endOffsetInBlock = endRwptr % BLOCK_SIZE; // exclusive

	int bytesRead = 0;

	buffer = (void*) malloc(BLOCK_SIZE);
	int addresses[NUM_ADDRESSES_INDIRECT]; // pointers inside indirect block
	int addressesInitialized = 0;
	int indirectBlockIndex;

	int i;
	for (i=startBlockIndex; i<= endBlockIndex; i++) {
		int tmp = calcByteAddress(i,0);

		memset(buffer, 0, BLOCK_SIZE);
		if (i > 11) {
			// indirect pointers

			if (!addressesInitialized) {
				// initialize block and read indirectAddresses
				read_blocks((*activeINode).indirectPointer, 1, buffer);
				memcpy(addresses, buffer, BLOCK_SIZE);
				memset(buffer, 0, BLOCK_SIZE);
				addressesInitialized = 1;
			}

			// write data to buf
			indirectBlockIndex = i-11-1;
			read_blocks(addresses[indirectBlockIndex], 1, buffer);

		} else {
			// direct pointers
      // read data to buf
      read_blocks((*activeINode).data_ptrs[i], 1, buffer);
    }
		if (i == startBlockIndex) {
			if (startBlockIndex == endBlockIndex) {
				memcpy(buf, buffer+startOffsetInBlock, endOffsetInBlock-startOffsetInBlock);
				bytesRead += endOffsetInBlock-startOffsetInBlock;
			} else {
				// read entire block
				memcpy(buf, buffer+startOffsetInBlock, BLOCK_SIZE-startOffsetInBlock);
				bytesRead += BLOCK_SIZE-startOffsetInBlock;
			}
		} else if (i == endBlockIndex) {
			memcpy(buf+bytesRead, buffer, endOffsetInBlock);
			bytesRead += endOffsetInBlock;
		} else {
			// read entire block and write BLOCK_SIZE bytes to buf + bytesRead
			memcpy(buf+bytesRead, buffer, BLOCK_SIZE);
			bytesRead += BLOCK_SIZE;
		}
	}

  // clean up
	(*tmpFd).rwptr += bytesRead;
	free(buffer);
	return bytesRead;
}

int sfs_fwrite(int fileID, const char *buf, int length) {
	if (length < 0) {
		printf("Length cannot be negative");
		return -1;
	}

	file_descriptor *tmpFd;
	tmpFd = &cachedFd[fileID];

	// check that file is open
	if ((*tmpFd).inodeIndex == -1) {
		printf("File is not open");
		return -1;
	}

	inode_t *activeINode;
	activeINode = (*tmpFd).inode;

	// calculate nb of blocks needed and index inside last block
	int rwptr = (*tmpFd).rwptr;
	int startBlockIndex = rwptr / BLOCK_SIZE;
	int startOffsetInBlock = rwptr % BLOCK_SIZE; // start writing here
	int endRwptr = rwptr + length;
	int endBlockIndex = endRwptr / BLOCK_SIZE;
	int endOffsetInBlock = endRwptr % BLOCK_SIZE; // exclusive

	int bytesWritten = 0;

	buffer = (void*) malloc(BLOCK_SIZE);
	int addresses[NUM_ADDRESSES_INDIRECT]; // pointers inside indirect block
	int addressesInitialized = 0;
	int indirectBlockIndex;
	int indirectBlockAddressModified = 0; // boolean
  int activeAddress;
	int fullError = 0;

	int i;
	for (i=startBlockIndex; i<= endBlockIndex; i++) {
		memset(buffer, 0, BLOCK_SIZE);
		if (i > 11) {
			// indirect pointers

			if (!addressesInitialized) {
				// reading indirect block, initializing addresses
				if ((*activeINode).indirectPointer == -1) {
					if (get_index() > 1023 || get_index() < 0) {
						printf("Reached max bitmap size");
						fullError = 1;
						break;
					}
					(*activeINode).indirectPointer = get_index();
					force_set_index((*activeINode).indirectPointer);
					int index;
					for (index=0; index<NUM_ADDRESSES_INDIRECT; index++)
						addresses[index] = -1;
					indirectBlockAddressModified = 1;
				} else {
					read_blocks((*activeINode).indirectPointer, 1, buffer);
					memcpy(addresses, buffer, BLOCK_SIZE);
					memset(buffer, 0, BLOCK_SIZE);
				}
				addressesInitialized = 1;
			}

			// write data to disk
			indirectBlockIndex = i-11-1;
			if (indirectBlockIndex >= NUM_ADDRESSES_INDIRECT) {
				printf("Reached max file size");
				fullError = 1;
				break;
			}
			if (addresses[indirectBlockIndex] == -1) {
				if (get_index() > 1023 || get_index() < 0) {
					printf("No more available blocks in bit map");
					fullError = 1;
					break;
				}
				addresses[indirectBlockIndex] = get_index();
				force_set_index(addresses[indirectBlockIndex]);
				indirectBlockAddressModified = 1;
			}
      activeAddress = addresses[indirectBlockIndex];

		} else {
			// direct pointers

			// write data to disk
			if ((*activeINode).data_ptrs[i] == -1) {
				if (get_index() > 1023 || get_index() < 0) {
					printf("No more available blocks in bit map");
					fullError = 1;
					break;
				}
				(*activeINode).data_ptrs[i] = get_index();
				force_set_index((*activeINode).data_ptrs[i]);
				if (i == startBlockIndex && startOffsetInBlock != 0)
					printf("(direct pointers) startOffsetInBlock should be 0. Investigate.");
			}

      activeAddress = (*activeINode).data_ptrs[i];

		}
    read_blocks(activeAddress, 1, buffer);
    if (i == startBlockIndex) {
    	if (startBlockIndex == endBlockIndex) {
    		// write (endOffsetInBlock-startOffsetInBlock) bytes from buf to end of buffer (buffer+startOffsetInBlock)
    		memcpy(buffer+startOffsetInBlock, buf, endOffsetInBlock-startOffsetInBlock);
    		bytesWritten += endOffsetInBlock-startOffsetInBlock;
    	} else {
    		// write (BLOCK_SIZE-startOffsetInBlock) bytes from buf to buffer+startOffsetInBlock
    		memcpy(buffer+startOffsetInBlock, buf, BLOCK_SIZE-startOffsetInBlock);
    		bytesWritten += BLOCK_SIZE-startOffsetInBlock;
    	}
    } else if (i == endBlockIndex) {
    	// write endOffsetInBlock bytes from buf+bytesWritten to buffer
    	memcpy(buffer, buf+bytesWritten, endOffsetInBlock);
    	if (length - bytesWritten != endOffsetInBlock)
    		printf("Investigate endOffsetInBlock in direct pointers");
    	bytesWritten += endOffsetInBlock;
    } else {
    	// write BLOCK_SIZE bytes from buf+bytesWritten to buffer
    	memcpy(buffer, buf+bytesWritten, BLOCK_SIZE);
    	bytesWritten += BLOCK_SIZE;
    }
    if (activeAddress < 0 || activeAddress > 1023) {
    	fullError = 1;
    	break;
    } else {
    	write_blocks(activeAddress, 1, buffer);
    }
	}

	// write indirectblock back to disk
	if (indirectBlockAddressModified) {
		memset(buffer, 0, BLOCK_SIZE);
		memcpy(buffer, addresses, BLOCK_SIZE);
		if ((*activeINode).indirectPointer < 0 || (*activeINode).indirectPointer > 1023) {
			printf("Why indirect block invalid value??");
			fullError = 1;
		} else {
			write_blocks((*activeINode).indirectPointer, 1, buffer);
		}
	}

	(*tmpFd).rwptr += bytesWritten;
	if ((*activeINode).size < (*tmpFd).rwptr) {
		(*activeINode).size = (*tmpFd).rwptr; // update size
	}
	free(buffer);
	if (saveINodeTable(0) < 0)
		printf("Failure(s) writing inode table to disk");
	if (saveFreeBitMap(0) < 0)
		printf("Failure(s) writing free bit map to disk");

	if (fullError)
		return -1;
	else
		return bytesWritten;
}

int sfs_fseek(int fileID, int loc) {
	if (fileID <= 0 || fileID > NUM_FILES) { // fileID 0 is reserved for root
		printf("fileID %d is invalid", fileID);
		return -1;
	}

	// make sure file already open
	if (cachedFd[fileID].inodeIndex == -1) {
		printf("File is not open");
		return -1;
	}

	if (loc < 0) {
		cachedFd[fileID].rwptr = 0;
	} else if (loc > cachedINodeTable[cachedFd[fileID].inodeIndex].size) {
		cachedFd[fileID].rwptr = cachedINodeTable[cachedFd[fileID].inodeIndex].size;
	} else {
		cachedFd[fileID].rwptr = loc;
	}

	return 0;
}

int sfs_remove(char *file) {
	// if (checkNameIsValid(file) < 0)
	// 	return -1;

	// find file by name - iterate root directory
	int i;
	int iNodeInd = -1;
	for (i=0; i<NUM_FILES; i++) {
		if (!strcmp(cachedFiles[i].name, file)) {
			iNodeInd = cachedFiles[i].num;
			break;
		}
	}

	if (i == NUM_FILES) {
		printf("File to be removed is not found");
		return -1;
	}

	// Remove file from cachedFd table
	for (i=1; i<INODE_TABLE_LEN; i++) {
		if (cachedFd[i].inodeIndex == iNodeInd) {
			cachedFd[i] = (file_descriptor) {-1, NULL, 0};
			totalFiles--;
		}
	}

	inode_t *activeINode;
	activeINode = &cachedINodeTable[iNodeInd];
	int endBlockIndex = (*activeINode).size / BLOCK_SIZE;
	int addresses[NUM_ADDRESSES_INDIRECT]; // pointers inside indirect block
	int addressesInitialized = 0;
	int indirectBlockIndex;
	int indirectBlockAddressModified = 0; // boolean

	buffer = (void*) malloc(BLOCK_SIZE);
	for (i=0; i<=endBlockIndex; i++) {
		memset(buffer, 0, BLOCK_SIZE);
		if (i>11) {
			// indirect pointers

			if (!addressesInitialized) {
				if ((*activeINode).indirectPointer == -1) {
					if ((*activeINode).size != 12*BLOCK_SIZE) {
						printf("Issue with size. Investigate");
						return -1;
					}
					break;
				}
				// initialize addresses
				read_blocks((*activeINode).indirectPointer, 1, buffer);
				memcpy(addresses, buffer, BLOCK_SIZE);
				memset(buffer, 0, BLOCK_SIZE);
				addressesInitialized = 1;
			}

			indirectBlockIndex = i-11-1;
			if (addresses[indirectBlockIndex] == -1) {
				if ((*activeINode).size != i*BLOCK_SIZE) {
					printf("Issue with size. Investigate");
					return -1;
				}
				break;
			}
			write_blocks(addresses[indirectBlockIndex], 1, buffer);
			rm_index(addresses[indirectBlockIndex]);
			addresses[indirectBlockIndex] = -1;
			indirectBlockAddressModified = 1;
		} else {
			// direct pointers
			write_blocks((*activeINode).data_ptrs[i], 1, buffer);
			rm_index((*activeINode).data_ptrs[i]);
			(*activeINode).data_ptrs[i] = -1;
		}
	}

	// write indirectblock back to disk
	if (indirectBlockAddressModified) {
		memset(buffer, 0, BLOCK_SIZE);
		memcpy(buffer, addresses, BLOCK_SIZE);
		write_blocks((*activeINode).indirectPointer, 1, buffer);
		indirectBlockAddressModified = 0;
	}

	rm_index((*activeINode).indirectPointer);
	(*activeINode).indirectPointer = -1;
	(*activeINode).size = -1;
	free(buffer);
	if (saveINodeTable(0) < 0)
		printf("Failure(s) writing inode table to disk");
	if (saveFreeBitMap(0) < 0)
		printf("Failure(s) writing free bit map to disk");
	if (saveRootDirectory(0) < 0)
		printf("Failure(s) writing root directory to disk");

	return 0;
}
