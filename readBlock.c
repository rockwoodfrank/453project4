#include "readBlock.h"

// NOTE: the readblock integer will be used as the file descriptor
int readBlock(int disk, int bNum, void *block)
{
    int byteOffset = bNum * BLOCKSIZE;
    int seekResult, readResult;

    // TODO: Error system

    // Translating the block number

    // Checking to make sure the file exists

    // Getting to the right offset
    if (lseek(disk, byteOffset, SEEK_SET) == -1)
    {
        // TODO: Failure stuff
        seekResult = -1;
        return seekResult;
    } else
    {
        // TODO: Success stuff
    }
    
    // Reading from the file
    if (read(disk, block, BLOCKSIZE) != BLOCKSIZE)
    {
        // TODO: error stuff
        return -1;
    }

    // Checking for a bad read


    return 0;  
}