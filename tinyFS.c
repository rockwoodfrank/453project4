#include "tinyFS.h"

int tfs_mkfs(char *filename, int nBytes) {

    /* open the disk */
    int disk_descriptor = openDisk(filename, nBytes);
    if(disk_descriptor < 0) {
        return -1;
    }

    /* Determine how many blocks the file will use */
    int number_of_blocks = nBytes / BLOCKSIZE;

    /* Ensure that the disk size is not too large */
    if(number_of_blocks > MAX_BLOCKS) {
        return -1;
    }

    /* Buffer to write data to newly opened disk*/
    uint8_t[BLOCKSIZE] buffer;
    memset(buffer, 0, BLOCKSIZE);
    
    /* Initialize free blocks */
    buffer[0] = 0x04;
    buffer[1] = 0x44
    buffer[2] = 0x02;
    for(int i=1; i< number_of_blocks; i++) {

        if(writeBlock(disk_descriptor, i, buffer) < 0) {
            return -1
        }

        /* If you aren't at the last block, link to the next free block */
        if(i+1 != number_of_blocks) {
            buffer[2]++;
        }
    }

    /* Initialize superblock */
    int last_free_block = buffer[2];
    memset(buffer, 0, BLOCKSIZE);
    buffer[0] = 0x01;
    buffer[1] = 0x44;
    buffer[2] = 0x01;

    /* Store the tail of chain of free blocks on the superblock */
    buffer[4] = last_free_block;

    if(writeBlock(disk_descriptor, 0, buffer) < 0) {
        return -1
    }

}