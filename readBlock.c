#include "readBlock.h"

// NOTE: the readblock integer will be used as the file descriptor
int readBlock(int disk, int bNum, void *block)
{
    printf("%d\n", BLOCKSIZE);
    int byteOffset = bNum * BLOCKSIZE;
    int seekResult, readResult;

    // TODO: Error system

    // Translating the block number

    // Checking to make sure the file exists

    // Getting to the right offset
    if (lseek(disk, byteOffset, SEEK_SET) != 0)
    {
        // TODO: Failure stuff
        int errorNum = -1;
        return errorNum;
    } else
    {
        // TODO: Success stuff
    }
    
    // Reading from the file
    readResult = read(disk, block, BLOCKSIZE);

    // Checking for a bad read


    return readResult;  
}