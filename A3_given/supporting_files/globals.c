#include "sfs_api.h"
#include "bitmap.h"
// #include "helpers.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <fuse.h>
#include <strings.h>
#include "disk_emu.h"


#define MCLEAN_ANGUS_DISK "sfs_disk.disk"
#define BLOCK_SIZE 1024
#define NUM_BLOCKS 1024  //maximum number of data blocks on the disk.
#define BITMAP_ROW_SIZE (NUM_BLOCKS/8) // this essentially mimcs the number of rows we have in the bitmap. we will have 128 rows.
#define NUM_INDIRECT_ADDRESSES (BLOCK_SIZE/sizeof(int))   //TODO
#define INODE_TABLE_LEN 100
#define TOTAL_NUM_BLOCKS_INODETABLE ((INODE_TABLE_LEN)*(sizeof(inode_t))/BLOCK_SIZE + (((INODE_TABLE_LEN)*(sizeof(inode_t)))%BLOCK_SIZE > 0))

#define NUM_FILES (INODE_TABLE_LEN-1)
#define TOTAL_NUM_BLOCKS_ROOTDIR ((NUM_FILES)*(sizeof(directory_entry))/BLOCK_SIZE + (((NUM_FILES)*(sizeof(directory_entry)))%BLOCK_SIZE > 0)) // getting ceiling value = 3

#define START_ADDRESS_OF_DATABLOCKS (1+TOTAL_NUM_BLOCKS_INODETABLE)


/////// State / Cached Variables ///////
//initialize all bits to high
// uint8_t free_bit_map[BITMAP_ROW_SIZE] = { [0 ... BITMAP_ROW_SIZE - 1] = UINT8_MAX };
struct superblock_t superblock;
void* buffer;
struct inode_t cachedINodeTable[INODE_TABLE_LEN];
struct directory_entry cachedFiles[NUM_FILES];
struct file_descriptor cachedFd[INODE_TABLE_LEN];

int totalFilesVisited;
int numTotalFiles;
