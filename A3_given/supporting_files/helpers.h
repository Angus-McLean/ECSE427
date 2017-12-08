int nameValid(const char* name);
void writeBlocksToBuf(void *data, int dataSize);
void readBlocksToBuf(int start_address, int nblocks);
int write_and_bitmap(int start_address, int nblocks, void *buffer);
int writeINodeTable(int updateBitMap);
int writeRootDirectory(int updateBitMap);
int writeFreeBitMap(int updateBitMap);
int calculateByteIndex(int blockIndex, int indexInBlock);
