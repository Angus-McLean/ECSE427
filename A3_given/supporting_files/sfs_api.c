
#include "sfs_api.h"
#include "bitmap.h"
// #include "helpers.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <fuse.h>
#include <strings.h>
#include "disk_emu.h"
// #define MCLEAN_ANGUS_DISK "sfs_disk.disk"
// #define BLOCK_SIZE 1024
// #define NUM_BLOCKS 1024  //maximum number of data blocks on the disk.
// #define BITMAP_ROW_SIZE (NUM_BLOCKS/8) // this essentially mimcs the number of rows we have in the bitmap. we will have 128 rows.
// #define NUM_ADDRESSES_INDIRECT (BLOCK_SIZE/sizeof(int))   //TODO
// #define INODE_TABLE_LEN 100
// #define TOTAL_NUM_BLOCKS_INODETABLE ((INODE_TABLE_LEN)*(sizeof(inode_t))/BLOCK_SIZE + (((INODE_TABLE_LEN)*(sizeof(inode_t)))%BLOCK_SIZE > 0))
//
// #define NUM_FILES (INODE_TABLE_LEN-1)
// #define TOTAL_NUM_BLOCKS_ROOTDIR ((NUM_FILES)*(sizeof(directory_entry))/BLOCK_SIZE + (((NUM_FILES)*(sizeof(directory_entry)))%BLOCK_SIZE > 0)) // getting ceiling value = 3
//
// #define START_ADDRESS_OF_DATABLOCKS (1+TOTAL_NUM_BLOCKS_INODETABLE)

/* macros */
#define FREE_BIT(_data, _which_bit) \
    _data = _data | (1 << _which_bit)

#define USE_BIT(_data, _which_bit) \
    _data = _data & ~(1 << _which_bit)


/////// State / Cached Variables ///////
//initialize all bits to high
// uint8_t free_bit_map[BITMAP_ROW_SIZE] = { [0 ... BITMAP_ROW_SIZE - 1] = UINT8_MAX };
// struct superblock_t superblock;
// void* buffer;
// struct inode_t cachedINodeTable[INODE_TABLE_LEN];
// struct directory_entry cachedFiles[NUM_FILES];
// struct file_descriptor cachedFd[INODE_TABLE_LEN];
//
// int filesVisited = 0; // for sfs_get_next_file_name
// int totalFiles = 0; // for sfs_get_next_file_name

#include "globals.c"
#include "helpers.h"

/////// Helper Functions ///////

// void writeBlocksToBuf(void *data, int dataSize) {
//   int blockedSize = (dataSize/BLOCK_SIZE + (dataSize%BLOCK_SIZE > 0)) * BLOCK_SIZE;
//   buffer = (void*) malloc(blockedSize);
// 	memset(buffer, 0, blockedSize);
// 	memcpy(buffer, data, dataSize);
// }
//
// void readBlocksToBuf(int start_address, int nblocks) {
//   buffer = (void*) malloc(nblocks*BLOCK_SIZE);
//   memset(buffer, 0, nblocks*BLOCK_SIZE);
//   read_blocks(start_address, nblocks, buffer);
// }
//
// int write_and_bitmap(int start_address, int nblocks, void *buffer){
//   int blocksWritten = write_blocks(start_address, nblocks, buffer);
//   for (int i=start_address; i<nblocks; i++)
//     force_set_index(i);
//   return blocksWritten;
// }
//
// int writeINodeTable(int updateBitMap) {
// 	// buffer = (void*) malloc(TOTAL_NUM_BLOCKS_INODETABLE*BLOCK_SIZE);
// 	// memset(buffer, 0, TOTAL_NUM_BLOCKS_INODETABLE*BLOCK_SIZE);
//   // memcpy(buffer, cachedINodeTable, (INODE_TABLE_LEN)*(sizeof(inode_t)));
//   writeBlocksToBuf(cachedINodeTable, (INODE_TABLE_LEN)*(sizeof(inode_t)));
//   int blocksWritten;
//   if(updateBitMap) {
//     blocksWritten = write_and_bitmap(1, TOTAL_NUM_BLOCKS_INODETABLE, buffer);
//   } else {
//     write_blocks(1, TOTAL_NUM_BLOCKS_INODETABLE, buffer);
//   }
// 	free(buffer);
// 	return blocksWritten;
// }
//
// int writeRootDirectory(int updateBitMap) {
// 	// buffer = (void*) malloc(TOTAL_NUM_BLOCKS_ROOTDIR*BLOCK_SIZE);
// 	// memset(buffer, 0, TOTAL_NUM_BLOCKS_ROOTDIR*BLOCK_SIZE);
//   // memcpy(buffer, cachedFiles, (NUM_FILES)*(sizeof(directory_entry)));
//   writeBlocksToBuf(cachedFiles, (NUM_FILES)*(sizeof(directory_entry)));
//   int blocksWritten;
//   if(updateBitMap) {
//     blocksWritten = write_and_bitmap(START_ADDRESS_OF_DATABLOCKS, TOTAL_NUM_BLOCKS_ROOTDIR, buffer);
//   } else {
//     write_blocks(START_ADDRESS_OF_DATABLOCKS, TOTAL_NUM_BLOCKS_ROOTDIR, buffer);
//   }
// 	free(buffer);
// 	return blocksWritten;
// }
//
// int writeFreeBitMap(int updateBitMap) { // write to disk
// 	// buffer = (void*) malloc(BLOCK_SIZE);
// 	// memset(buffer, 1, BLOCK_SIZE);
//   // memcpy(buffer, free_bit_map, (SIZE)*(sizeof(uint8_t)));
//   writeBlocksToBuf(free_bit_map, (SIZE)*(sizeof(uint8_t)));
//   int blocksWritten;
//   if(updateBitMap) {
//     blocksWritten = write_and_bitmap(NUM_BLOCKS-1, 1, buffer);
//   } else {
//     write_blocks(NUM_BLOCKS-1, 1, buffer);
//   }
// 	free(buffer);
// 	return blocksWritten;
// }
//
// // TODO
// int calculateByteIndex(int blockIndex, int indexInBlock) {
// 	return BLOCK_SIZE*blockIndex+indexInBlock;
// }

int filesVisited = 0; // for sfs_get_next_file_name
int totalFiles = 0; // for sfs_get_next_file_name

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
    // force_set_index(0);

    // initialize inode table with empty inodes
    cachedINodeTable[0] = (inode_t) {777, 0, 0, 0, 0, {START_ADDRESS_OF_DATABLOCKS, START_ADDRESS_OF_DATABLOCKS+1, START_ADDRESS_OF_DATABLOCKS+2, -1, -1, -1, -1, -1, -1, -1, -1, -1}, -1};
    for (int i=1; i<INODE_TABLE_LEN; i++)
      cachedINodeTable[i] = (inode_t) {777, 0, 0, 0, -1, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, -1};
    if (writeINodeTable(1) < 0) printf("Failed to writed inode table\n");

    // init directory entries
		for (i=0; i<NUM_FILES; i++) {
			cachedFiles[i].num = -1;
			memset(cachedFiles[i].name, '\0', sizeof(cachedFiles[i].name));
		}
		if (writeRootDirectory(1) < 0) printf("Failed to write root directory\n");


    // writing freeBitMap to disk
		if (writeFreeBitMap(1) < 0) printf("Failed to write freeBitMap to disk\n");
	} else {
    init_disk(MCLEAN_ANGUS_DISK, BLOCK_SIZE, NUM_BLOCKS);

    // read freebitmap
		// buffer = (void*) malloc(BLOCK_SIZE);
		// memset(buffer, 1, BLOCK_SIZE);
		// read_blocks(NUM_BLOCKS-1, 1, buffer);
    readBlocksToBuf(NUM_BLOCKS-1, 1);
    memcpy(free_bit_map, buffer, (SIZE)*(sizeof(uint8_t)));
		free(buffer);
		force_set_index(NUM_BLOCKS-1);


		// read superblock
		// buffer = (void*) malloc(BLOCK_SIZE);
		// memset(buffer, 0, BLOCK_SIZE);
		// read_blocks(0, 1, buffer);
    readBlocksToBuf(0, 1);
    memcpy(&superblock, buffer, sizeof(superblock_t));
		free(buffer);
		force_set_index(0);


		// read inode table
		// buffer = (void*) malloc(TOTAL_NUM_BLOCKS_INODETABLE*BLOCK_SIZE);
		// memset(buffer, 0, TOTAL_NUM_BLOCKS_INODETABLE*BLOCK_SIZE);
		// read_blocks(1, TOTAL_NUM_BLOCKS_INODETABLE, buffer);
    readBlocksToBuf(1, TOTAL_NUM_BLOCKS_INODETABLE);
    memcpy(cachedINodeTable, buffer, INODE_TABLE_LEN*sizeof(inode_t));
		free(buffer);
		for (i=0; i<TOTAL_NUM_BLOCKS_INODETABLE; i++)
			force_set_index(i+1);


		// read root directory
		// buffer = (void*) malloc(TOTAL_NUM_BLOCKS_ROOTDIR*BLOCK_SIZE);
		// memset(buffer, 0, TOTAL_NUM_BLOCKS_ROOTDIR*BLOCK_SIZE);
		// read_blocks(START_ADDRESS_OF_DATABLOCKS, TOTAL_NUM_BLOCKS_ROOTDIR, buffer);
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
  // if (nameValid(path) < 0)
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
	// if (nameValid(name) < 0)
	// 	return -1;


	int iNodeIndex = -1;
	int i;
  // iterate to find file
	for (i=0; i<NUM_FILES; i++) {
    // printf("Iterating %d, %s\n", i, cachedFiles[i].name);
		if (!strcmp(cachedFiles[i].name, name)) {
			iNodeIndex = cachedFiles[i].num;
			break;
		}
	}

	if (iNodeIndex > -1) {
    // file exists at
		for (i=1; i<INODE_TABLE_LEN; i++) {
			if (cachedFd[i].inodeIndex == iNodeIndex) {
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
				iNodeIndex = i;
				break;
			}
		}
		if (iNodeIndex < 0) {
			printf("Investigate!!!!\n");
			return -1;
		}

		strcpy(cachedFiles[fileIndex].name, name);
		cachedFiles[fileIndex].num = iNodeIndex;
		cachedINodeTable[iNodeIndex].size = 0;

		if (writeINodeTable(0) < 0)
			printf("Failure(s) writing iNode to disk");
		if (writeRootDirectory(0) < 0)
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

	int rwptr = cachedINodeTable[iNodeIndex].size;
	cachedFd[fdIndex] = (file_descriptor) {iNodeIndex, &cachedINodeTable[iNodeIndex], rwptr};
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

	file_descriptor *myFd;
	myFd = &cachedFd[fileID];

	// check that file is open
	if ((*myFd).inodeIndex == -1) {
		printf("File is not open");
		return -1;
	}

	inode_t *myINode;
	myINode = (*myFd).inode;

	// calculate nb of blocks needed and index inside last block
	int rwptr = (*myFd).rwptr;
	int startBlockIndex = rwptr / BLOCK_SIZE;
	int startIndexInBlock = rwptr % BLOCK_SIZE; // start writing here
	int endRwptr;
	if (rwptr+length > (*myINode).size) {
		endRwptr = (*myINode).size;
	} else {
		endRwptr = rwptr + length;
	}
	int endBlockIndex = endRwptr / BLOCK_SIZE;
	int endIndexInBlock = endRwptr % BLOCK_SIZE; // exclusive

	int bytesRead = 0;

	buffer = (void*) malloc(BLOCK_SIZE);
	int addresses[NUM_ADDRESSES_INDIRECT]; // pointers inside indirect block
	int addressesInitialized = 0;
	int indirectBlockIndex;

	int i;
	for (i=startBlockIndex; i<= endBlockIndex; i++) {
		int tmp = calculateByteIndex(i,0);
		if (tmp >= (*myINode).size) {
			printf("File cannot be read past its size"); // should not pass here
			printf("SOMETHING WRONG - i: %d myINode.size %d", i, (*myINode).size);
			break;
		}
		memset(buffer, 0, BLOCK_SIZE);
		if (i > 11) {
			// indirect pointers

			if (!addressesInitialized) {
				// reading indirect block, initializing addresses
				read_blocks((*myINode).indirectPointer, 1, buffer);
				memcpy(addresses, buffer, BLOCK_SIZE);
				memset(buffer, 0, BLOCK_SIZE);
				addressesInitialized = 1;
			}

			// write data to buf
			indirectBlockIndex = i-11-1;
			if (addresses[indirectBlockIndex] == -1) {
					printf("Address of block has not been found. Investigate"); // bytesIndex < muINode.size so it should be found
					break;
			}
			read_blocks(addresses[indirectBlockIndex], 1, buffer);
			if (i == startBlockIndex) {
				if (startBlockIndex == endBlockIndex) {
					// read buffer from startIndexInBlock to endIndexInBlock and write (endIndexInBlock-startIndexInBlock) bytes to buf
					memcpy(buf, buffer+startIndexInBlock, endIndexInBlock-startIndexInBlock);
					bytesRead += endIndexInBlock-startIndexInBlock;
				} else {
					// read buffer from startIndexInBlock to end of block and write (BLOCK_SIZE-startIndexInBlock) bytes to buf
					memcpy(buf, buffer+startIndexInBlock, BLOCK_SIZE-startIndexInBlock);
					bytesRead += BLOCK_SIZE-startIndexInBlock;
				}
			} else if (i == endBlockIndex) {
				// read until endIndexInBlock (or size) and write endIndexInBlock bytes to buf + bytesRead
				if (calculateByteIndex(i, endIndexInBlock) > (*myINode).size)
					printf("SOMETHING WRONG - i: %d endIndexInBlock: %d myINode.size %d", i, endIndexInBlock, (*myINode).size);
				// read until endIndexInBlock
				memcpy(buf+bytesRead, buffer, endIndexInBlock);
				if (length - bytesRead != endIndexInBlock)
					printf("Investigate endIndexInBlock in indirect pointers");
				bytesRead += endIndexInBlock;

			} else {
				// read entire block and write BLOCK_SIZE bytes to buf + bytesRead
				memcpy(buf+bytesRead, buffer, BLOCK_SIZE);
				bytesRead += BLOCK_SIZE;
			}
		} else {
			// direct pointers

			// write data to buf
			if ((*myINode).data_ptrs[i] == -1) {
					printf("Address of block has not been found. Investigate"); // bytesIndex < myINode.size so it should be found
					break;
			}
			read_blocks((*myINode).data_ptrs[i], 1, buffer);
			if (i == startBlockIndex) {
				if (startBlockIndex == endBlockIndex) {
					// read buffer from startIndexInBlock to endIndexInBlock and write (endIndexInBlock-startIndexInBlock) bytes to buf
					memcpy(buf, buffer+startIndexInBlock, endIndexInBlock-startIndexInBlock);
					bytesRead += endIndexInBlock-startIndexInBlock;
				} else {
					// read buffer from startIndexInBlock to end of block and write (BLOCK_SIZE-startIndexInBlock) bytes to buf
					memcpy(buf, buffer+startIndexInBlock, BLOCK_SIZE-startIndexInBlock);
					bytesRead += BLOCK_SIZE-startIndexInBlock;
				}
			} else if (i == endBlockIndex) {
				// read until endIndexInBlock (or size) and write endIndexInBlock bytes to buf + bytesRead
				if (calculateByteIndex(i, endIndexInBlock) > (*myINode).size)
					printf("SOMETHING WRONG - i: %d endIndexInBlock: %d myINode.size %d", i, endIndexInBlock, (*myINode).size);
				// read until endIndexInBlock
				memcpy(buf+bytesRead, buffer, endIndexInBlock);
				if (length - bytesRead != endIndexInBlock)
					printf("Investigate endIndexInBlock in direct pointers");
				bytesRead += endIndexInBlock;
			} else {
				// read entire block and write BLOCK_SIZE bytes to buf + bytesRead
				memcpy(buf+bytesRead, buffer, BLOCK_SIZE);
				bytesRead += BLOCK_SIZE;
			}
		}
	}

	(*myFd).rwptr += bytesRead;
	free(buffer);
	return bytesRead;
}

int sfs_fwrite(int fileID, const char *buf, int length) {
	if (length < 0) {
		printf("Length cannot be negative");
		return -1;
	}

	file_descriptor *myFd;
	myFd = &cachedFd[fileID];

	// check that file is open
	if ((*myFd).inodeIndex == -1) {
		printf("File is not open");
		return -1;
	}

	inode_t *myINode;
	myINode = (*myFd).inode;

	// calculate nb of blocks needed and index inside last block
	int rwptr = (*myFd).rwptr;
	int startBlockIndex = rwptr / BLOCK_SIZE;
	int startIndexInBlock = rwptr % BLOCK_SIZE; // start writing here
	int endRwptr = rwptr + length;
	int endBlockIndex = endRwptr / BLOCK_SIZE;
	int endIndexInBlock = endRwptr % BLOCK_SIZE; // exclusive

	int bytesWritten = 0;

	buffer = (void*) malloc(BLOCK_SIZE);
	int addresses[NUM_ADDRESSES_INDIRECT]; // pointers inside indirect block
	int addressesInitialized = 0;
	int indirectBlockIndex;
	int indirectBlockAddressModified = 0; // boolean

	int fullError = 0;

	int i;
	for (i=startBlockIndex; i<= endBlockIndex; i++) {
		memset(buffer, 0, BLOCK_SIZE);
		if (i > 11) {
			// indirect pointers

			if (!addressesInitialized) {
				// reading indirect block, initializing addresses
				if ((*myINode).indirectPointer == -1) {
					if (get_index() > 1023 || get_index() < 0) {
						printf("No more available blocks in bit map");
						fullError = 1;
						break;
					}
					(*myINode).indirectPointer = get_index();
					force_set_index((*myINode).indirectPointer);
					int index;
					for (index=0; index<NUM_ADDRESSES_INDIRECT; index++)
						addresses[index] = -1;
					indirectBlockAddressModified = 1;
				} else {
					read_blocks((*myINode).indirectPointer, 1, buffer);
					memcpy(addresses, buffer, BLOCK_SIZE);
					memset(buffer, 0, BLOCK_SIZE);
				}
				addressesInitialized = 1;
			}

			// write data to disk
			indirectBlockIndex = i-11-1;
			if (indirectBlockIndex >= NUM_ADDRESSES_INDIRECT) {
				printf("Max file size has been reached");
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
			read_blocks(addresses[indirectBlockIndex], 1, buffer);
			if (i == startBlockIndex) {
				if (startBlockIndex == endBlockIndex) {
					// write (endIndexInBlock-startIndexInBlock) bytes from buf to end of buffer (buffer+startIndexInBlock)
					memcpy(buffer+startIndexInBlock, buf, endIndexInBlock-startIndexInBlock);
					bytesWritten += endIndexInBlock-startIndexInBlock;
				} else {
					// write (BLOCK_SIZE-startIndexInBlock) bytes from buf to end of buffer (buffer+startIndexInBlock)
					memcpy(buffer+startIndexInBlock, buf, BLOCK_SIZE-startIndexInBlock);
					bytesWritten += BLOCK_SIZE-startIndexInBlock;
				}
			} else if (i == endBlockIndex) {
				// write endIndexInBlock bytes from buf+bytesWritten to beginning of buffer
				memcpy(buffer, buf+bytesWritten, endIndexInBlock);
				if (length - bytesWritten != endIndexInBlock)
					printf("Investigate endIndexInBlock in indirect pointers");
				bytesWritten += endIndexInBlock;
			} else {
				// write BLOCK_SIZE bytes from buf+bytesWritten to buffer
				memcpy(buffer, buf+bytesWritten, BLOCK_SIZE);
				bytesWritten += BLOCK_SIZE;
			}
			if (addresses[indirectBlockIndex] < 0 || addresses[indirectBlockIndex] > 1023) {
				printf("Why indirect pointer invalid value??");
				fullError = 1;
				break;
			} else {
				write_blocks(addresses[indirectBlockIndex], 1, buffer);
			}
		} else {
			// direct pointers

			// write data to disk
			if ((*myINode).data_ptrs[i] == -1) {
				if (get_index() > 1023 || get_index() < 0) {
					printf("No more available blocks in bit map");
					fullError = 1;
					break;
				}
				(*myINode).data_ptrs[i] = get_index();
				force_set_index((*myINode).data_ptrs[i]);
				if (i == startBlockIndex && startIndexInBlock != 0)
					printf("(direct pointers) startIndexInBlock should be 0. Investigate.");
			}
			read_blocks((*myINode).data_ptrs[i], 1, buffer);
			if (i == startBlockIndex) {
				if (startBlockIndex == endBlockIndex) {
					// write (endIndexInBlock-startIndexInBlock) bytes from buf to end of buffer (buffer+startIndexInBlock)
					memcpy(buffer+startIndexInBlock, buf, endIndexInBlock-startIndexInBlock);
					bytesWritten += endIndexInBlock-startIndexInBlock;
				} else {
					// write (BLOCK_SIZE-startIndexInBlock) bytes from buf to buffer+startIndexInBlock
					memcpy(buffer+startIndexInBlock, buf, BLOCK_SIZE-startIndexInBlock);
					bytesWritten += BLOCK_SIZE-startIndexInBlock;
				}
			} else if (i == endBlockIndex) {
				// write endIndexInBlock bytes from buf+bytesWritten to buffer
				memcpy(buffer, buf+bytesWritten, endIndexInBlock);
				if (length - bytesWritten != endIndexInBlock)
					printf("Investigate endIndexInBlock in direct pointers");
				bytesWritten += endIndexInBlock;
			} else {
				// write BLOCK_SIZE bytes from buf+bytesWritten to buffer
				memcpy(buffer, buf+bytesWritten, BLOCK_SIZE);
				bytesWritten += BLOCK_SIZE;
			}
			if ((*myINode).data_ptrs[i] < 0 || (*myINode).data_ptrs[i] > 1023) {
				printf("Why data pointer invalid value??");
				fullError = 1;
				break;
			} else {
				write_blocks((*myINode).data_ptrs[i], 1, buffer);
			}
		}
	}

	// write indirectblock back to disk
	if (indirectBlockAddressModified) {
		memset(buffer, 0, BLOCK_SIZE);
		memcpy(buffer, addresses, BLOCK_SIZE);
		if ((*myINode).indirectPointer < 0 || (*myINode).indirectPointer > 1023) {
			printf("Why indirect block invalid value??");
			fullError = 1;
		} else {
			write_blocks((*myINode).indirectPointer, 1, buffer);
		}
	}

	(*myFd).rwptr += bytesWritten;
	if ((*myINode).size < (*myFd).rwptr) {
		(*myINode).size = (*myFd).rwptr; // update size
	}
	free(buffer);
	if (writeINodeTable(0) < 0)
		printf("Failure(s) writing inode table to disk");
	if (writeFreeBitMap(0) < 0)
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
	// if (nameValid(file) < 0)
	// 	return -1;

	// find file by name - iterate root directory
	int i;
	int iNodeIndex = -1;
	for (i=0; i<NUM_FILES; i++) {
		if (!strcmp(cachedFiles[i].name, file)) {
			iNodeIndex = cachedFiles[i].num;
			break;
		}
	}

	if (i == NUM_FILES) {
		printf("File to be removed is not found");
		return -1;
	}

	// Remove file from cachedFd table
	for (i=1; i<INODE_TABLE_LEN; i++) {
		if (cachedFd[i].inodeIndex == iNodeIndex) {
			cachedFd[i] = (file_descriptor) {-1, NULL, 0};
			totalFiles--;
		}
	}

	inode_t *myINode;
	myINode = &cachedINodeTable[iNodeIndex];
	int endBlockIndex = (*myINode).size / BLOCK_SIZE;
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
				if ((*myINode).indirectPointer == -1) {
					if ((*myINode).size != 12*BLOCK_SIZE) {
						printf("Issue with size. Investigate");
						return -1;
					}
					break;
				}
				// initialize addresses
				read_blocks((*myINode).indirectPointer, 1, buffer);
				memcpy(addresses, buffer, BLOCK_SIZE);
				memset(buffer, 0, BLOCK_SIZE);
				addressesInitialized = 1;
			}

			indirectBlockIndex = i-11-1;
			if (addresses[indirectBlockIndex] == -1) {
				if ((*myINode).size != i*BLOCK_SIZE) {
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
			write_blocks((*myINode).data_ptrs[i], 1, buffer);
			rm_index((*myINode).data_ptrs[i]);
			(*myINode).data_ptrs[i] = -1;
		}
	}

	// write indirectblock back to disk
	if (indirectBlockAddressModified) {
		memset(buffer, 0, BLOCK_SIZE);
		memcpy(buffer, addresses, BLOCK_SIZE);
		write_blocks((*myINode).indirectPointer, 1, buffer);
		indirectBlockAddressModified = 0;
	}

	rm_index((*myINode).indirectPointer);
	(*myINode).indirectPointer = -1;
	(*myINode).size = -1;
	free(buffer);
	if (writeINodeTable(0) < 0)
		printf("Failure(s) writing inode table to disk");
	if (writeFreeBitMap(0) < 0)
		printf("Failure(s) writing free bit map to disk");
	if (writeRootDirectory(0) < 0)
		printf("Failure(s) writing root directory to disk");

	return 0;
}
