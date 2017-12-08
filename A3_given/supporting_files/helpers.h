int checkNameIsValid(const char* name);
void writeBlocksToBuf(void *data, int dataSize);
void readBlocksToBuf(int start_address, int nblocks);
int write_and_bitmap(int start_address, int nblocks, void *buffer);
int saveINodeTable(int updateBitMap);
int saveRootDirectory(int updateBitMap);
int saveFreeBitMap(int updateBitMap);
int calcByteAddress(int blockIndex, int indexInBlock);
