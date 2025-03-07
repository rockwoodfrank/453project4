#include "tinyFS.h"

tinyFS* mounted = NULL;

/* 
- fd_table: gloabl array of inodes for open file descriptors on memory
    > fd_table[fd] = inode corresponding to fd
    > value of 0 means invalid fd / fd is available to be set
- fd_table_index: the next available index of fd_table once updated
    > updated by _update_fd_table_index()
*/
uint8_t fd_table[FD_TABLESIZE]; 
int fd_table_index = 0;

/* internal helper functions */
int     _update_fd_table_index();
uint8_t _pop_free_block();
int     _free_block(uint8_t block_addr);

int tfs_mkfs(char *filename, int nBytes) {
    /* error if given 0 bytes */
    if(nBytes == 0) {
        return -1;
    }

    /* open the disk */
    int disk_descriptor = openDisk(filename, nBytes);
    if(disk_descriptor < 0) {
        return -1;
    }

    /* find how many blocks the file will use and ensure that the disk size is not too large */
    int number_of_blocks = nBytes / BLOCKSIZE;
    if(number_of_blocks > MAX_BLOCKS) {
        return -1;
    }

    /* buffer to write data to newly opened disk*/
    uint8_t buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE);
    int last_free_block;
    
    /* initialize free blocks */
    buffer[BLOCK_TYPE] = FREE;
    buffer[SAFETY_BYTE] = SAFETY_HEX;
    buffer[FREE_PTR] = 0x01;
    for(int i = 1; i < number_of_blocks; i++) {

        /* if not at the last block, link to the next free block */
        if(i+1 != number_of_blocks) {
            buffer[FREE_PTR]++;

        /* if at the last block, store the last free block */
        } else {
            last_free_block = buffer[FREE_PTR];
            buffer[FREE_PTR] = 0x00;
        }

        /* update the block with the correct type, safety, and pointer bytes */
        if(writeBlock(disk_descriptor, i, buffer) < 0) {
            return -1;
        }
    }

    /* Initialize superblock */
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCK_TYPE] = SUPERBLOCK;
    buffer[SAFETY_BYTE] = SAFETY_HEX;
    buffer[FREE_PTR] = 0x01;
    buffer[4] = last_free_block;

    if(writeBlock(disk_descriptor, SUPERBLOCK_DISKLOC, buffer) < 0) {
        return -1;
    }

    return 0;
}

int tfs_mount(char* diskname) {
    /* open the given disk */
    int diskNum = openDisk(diskname, 0);
    if (diskNum < 0) {
        return -1;
    }

    /* Returning an error if the file isn't formatted properly */
    /* Done by making sure byte 1 of the superblock is 0x44 */
    // SYDNOTE: do we need to check the 0x44 and block type for each block in the disk file?
    // + make sure the disk file is perfectly divisible by blocksize ..? or nah
    uint8_t buffer[BLOCKSIZE];
    readBlock(diskNum, SUPERBLOCK_DISKLOC, buffer);
    uint8_t byte0 = buffer[BLOCK_TYPE];
    uint8_t byte1 = buffer[SAFETY_BYTE];
    if (byte0 != SUPERBLOCK || byte1 != SAFETY_HEX) {
        return -1;
    }

    /* if there currently is a disk file mounted, unmount it */
    if (mounted != NULL) {
        tfs_unmount();
    }

    /* Initialize a new tinyFS object */
    // SYDNOTE: do we need to malloc since it is global? 
    // cuz do we want to have to rely of the user to unmount when its just storing a string and an int
    mounted = (tinyFS *) malloc(sizeof(tinyFS));
    mounted->name = diskname;
    mounted->diskNum = diskNum;

    /* reset the fd table */
    memset(fd_table, 0, FD_TABLESIZE);

    return 0;
}

int tfs_unmount() {
    if (mounted == NULL) {
        return -1;
    }
        
    /* Free the mounted variable and change it to a null pointer */
    int returnVal = closeDisk(mounted->diskNum);

    free(mounted);
    mounted = NULL;

    /* TODO: Make sure the file is unmounted "cleanly" */
    return returnVal;
}

fileDescriptor tfs_openFile(char *name) {
    if(name == NULL) {
        return -1;
    }

    /* truncate name to be the first 8 chars */
    int z = 0;
    char name_trunc[9]; //SYDNOTE: FILENAME+LENGTH + 1 ?
    memset(name_trunc, 0, 9);
    while(z < 8 && name[z] != '\0') {
        name_trunc[z] = name[z];
        z++;
    }

    /* Get the superblock */
    uint8_t superblock[BLOCKSIZE];
    readBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock);

    /* Buffer to hold inode data */
    char inode_buffer[BLOCKSIZE];
    
    /* Look through the inode pointers in the superblock */
    for(int i=5; i<MAX_INODES; i++) {
        memset(inode_buffer, 0, BLOCKSIZE);

        /* Break out if we have an empty block, meaning the file doesn't exist */
        // SYDNOTE: if we remove an inode and don't fill the hole this will cause problems
        // might just need to loop through all 5-255 bytes in superblock, check if is inode then proceed
        if(!superblock[i]) {
            break;
        }

        /* Grab the name from the inode buffer */
        readBlock(mounted->diskNum, superblock[i], inode_buffer);
        char* filename = inode_buffer + 4; //SYDNOTE #DEFINE FILELOC 4 might be more clean

        /* if found the file, get a new fd and update the fd table*/
        if(strcmp(name_trunc, filename) == 0) {
            printf("Found file at inode %d\n", superblock[i]);
            if(_update_fd_table_index() < 0) {
                return -1;
            }

            fd_table[fd_table_index] = superblock[i];
            return fd_table_index;
        }
    }

    /* File not found, so create inode for it */
    // TODO: How is this updated? Do we need to make a function for that? What should we 
    //do when we delete a file? Move every inode over?
    uint8_t next_free_block = _pop_free_block();
    readBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock);

    /* if there is an available free block... */
    if(next_free_block) {
        
        /* find the first empty byte in the superblock and add the inode */
        for(int i = 5; i < MAX_INODES; i++) {
            if(!superblock[i]) {
                superblock[i] = next_free_block;
                break;
            }
        }

        /* put name of file on the inode and information bytes */
        int z = 0;
        while(name_trunc[z] != '\000') {
            inode_buffer[z+4] = name[z]; // SYDNOTE DEFINE FILENAMELOC ?
            z++;
        }
        inode_buffer[BLOCK_TYPE] = INODE;
        inode_buffer[SAFETY_BYTE] = SAFETY_HEX;

        // find the next available fd and set it to the next 
        if(_update_fd_table_index() < 0) {
            return -1;
        }
        fd_table[fd_table_index] = next_free_block;

        /* turn the free block into an inode */
        if(writeBlock(mounted->diskNum, next_free_block, inode_buffer) < 0) {
            return -1;
        }

        /* update the superblock */
        if(writeBlock(mounted->diskNum, 0, superblock) < 0) {
            return -1;
        }

        return fd_table_index;
    } 

    /* error if no available free block */
    return -1;
}

// SYDNOTE: surely there is more to this than that
// need to clean the inode if no other fd's point to it..?
// if none left -> deleteFile()
int tfs_closeFile(fileDescriptor FD) {
    /* if there is an entry, remove entry */
    if(fd_table[FD]) {
        fd_table[FD] = 0;
        return 0;
    }

    /* return error if there is no entry */
    return -1;
}

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire
file’s content, to the file system. Previous content (if any) will be
completely lost. Sets the file pointer to 0 (the start of file) when
done. Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
    // Grab the block's inode
    uint8_t inode[BLOCKSIZE]; 
    readBlock(mounted->diskNum, fd_table[FD], inode);

    /* store the file size in the inode */
    inode[13] = (size >> 3) & 0xFF;
    inode[14] = (size >> 2) & 0xFF;
    inode[15] = (size >> 1) & 0xFF;
    inode[16] = size & 0xFF;

    // Resetting the direct blocks so the data is "lost" since nothing is pointing to them
    // Counter for clearing
    int i = 0;
    uint8_t dir_block = inode[DBLOCKS + i++];
    while(dir_block != 0x0) {
        // Allocating each block as "free" and adding them to the linked list
        _free_block(dir_block);
        dir_block = inode[DBLOCKS + i++];
    }

    // Determining the amount of blocks to be written. A plus one at the end for data outside the 256 byte margin.
    // NOTE TO PROGRAMMER: I set this to size-1 so write of 256 bytes(or any number on the line)
    // will not take up extra blocks. Might cause problems in the future.
    //SYDNOTE: can only do that if the last byte is a /0 ..?
    // int numBlocks = ((size-1) / DATA_SPACE) + 1;
    int numBlocks = (size / DATA_SPACE) + 1;
    // Writing those blocks to the file
    uint8_t temp_addr;
    uint8_t temp_block[BLOCKSIZE];
    // A value to keep track of where we are in the buffer
    int bufferHead = 0;
    for (int i = 0; i < numBlocks; i++) {
        temp_addr = _pop_free_block();
        if (temp_addr) {
            readBlock(mounted->diskNum, temp_addr, temp_block);
            // Update important data
            temp_block[BLOCK_TYPE] = FILEEX;
            //temp_block[2] = inode; TODO: if we want to link up parents, not sure right now

            // Copy the buffer data over to the block
            // TODO: Change i's value into a macro? // SYDNOTE: could be same as one used for FILENAMELOC (rename if use the same for both)
            for (int i = 4; i < BLOCKSIZE; i++) {
                temp_block[i] = buffer[bufferHead++];
            }
            writeBlock(mounted->diskNum, temp_addr, temp_block);
        }
        // TODO: Error checking(probably a full disk)
    }

    /* set the file offset to be 0 */
    tfs_seek(FD, 0); 

    return 0;
}

int tfs_readByte(fileDescriptor FD, char* buffer) {
    // Grab the block's inode
    uint8_t inode[BLOCKSIZE]; 
    readBlock(mounted->diskNum, fd_table[FD], inode);

    // get file offset from inode block and convert to block & block offset
    // SYDNOTE: could make this a for loop situation to use macros but that might be annoying/hard to read
    int offset = (inode[17] << 3) + (inode[18] << 2) + (inode[19] << 1) + inode[20];
    int block_num = offset / BLOCKSIZE;
    int block_offset = offset % BLOCKSIZE;

    // increment the offset and make sure it is in the file
    int seek_status = tfs_seek(FD, offset++);
    if (seek_status < 0) {
        return seek_status;
    }

    // Grab the data block
    uint8_t data_block_num = inode[DBLOCKS + block_num];
    uint8_t data_block[BLOCKSIZE]; 
    readBlock(mounted->diskNum, data_block_num, data_block);

    // store the byte at the offset into the given buffer
    buffer[0] =  data_block[block_offset];
    
    return 0;
}

int tfs_seek(fileDescriptor FD, int offset) {
    // Grab the inode
    uint8_t inode[BLOCKSIZE]; 
    readBlock(mounted->diskNum, fd_table[FD], inode);

    // get file size
    int size = (inode[13] << 3) + (inode[14] << 2) + (inode[15] << 1) + inode[16];

    // make sure the offset is in the file
    if (offset >= size || offset < 0) {
        return -1;
    } 

    // store the offset in the file inode
    inode[17] = (offset >> 3) & 0xFF;
    inode[18] = (offset >> 2) & 0xFF;
    inode[19] = (offset >> 1) & 0xFF;
    inode[20] = offset & 0xFF;

    return 0;
}

/* Helper function for to get the the next available fd table index */
int _update_fd_table_index() {
    int original = fd_table_index;

    /* iterate to the end of the list until next available fd */
    for(int i = fd_table_index; i < FD_TABLESIZE; i++) {
        if(fd_table[fd_table_index] == 0) {
            return fd_table_index;
        } else {
            fd_table_index++;
        }
    }

    /* Start over until up until original starting point */
    fd_table_index = 0;
    for(int i=0; i < original; i++) {
        if(fd_table[fd_table_index] == 0) {
            return fd_table_index;
        } else {
            fd_table_index++;
        }
    }

    /* if you made it this far, there are no available file descriptors */
    return -1;
}

// Pop and return the next free block, and replace the superblock index
// with that block's next block. Should return 0 if no more free blocks exist.
uint8_t _pop_free_block() {
    // Grab the superblock. This is done locally as some functions may not
    // need to store the superblock so this function does it just in case
    uint8_t superblock[BLOCKSIZE];
    readBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock);

    uint8_t newBlock[BLOCKSIZE];
    uint8_t next_free_block = superblock[FREE_PTR];
    /* then, grab the address for the next free inode*/
    readBlock(mounted->diskNum, next_free_block, newBlock);
    // TODO: return an error on a bad read
    /* update next free block */
    superblock[FREE_PTR] = newBlock[FREE_PTR];
    writeBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock);
    return next_free_block;
}

/* free_block turns the given block into a free block, and also adds
    it to the list of free blocks. Returns a 0 on success or -1 on error*/
int _free_block(uint8_t block_addr) {
    // Grab the superblock
    uint8_t superblock[BLOCKSIZE];
    readBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock);

    // Change block type
    uint8_t clean_block[BLOCKSIZE];
    clean_block[BLOCK_TYPE] = FREE;
    clean_block[SAFETY_BYTE] = SAFETY_HEX;
    clean_block[FREE_PTR] = superblock[FREE_PTR];
    writeBlock(mounted->diskNum, block_addr, clean_block);
    
    // Change free list
    superblock[FREE_PTR] = block_addr;
    writeBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock);
    // TODO: Error checking
    return 0;
}

/* Uncomment and run this block if you want to test */
int main(int argc, char* argv[]) {
    if(argc > 1) {
        tfs_mkfs("SydneysDisk.dsk", 8192);
    }

    if(tfs_mount("SydneysDisk.dsk") < 0) {
        printf("Failed to mount Sydney's disk\n");
        return -1;
    }

    /* Existing files */
    int fd = tfs_openFile("test1XXXIshouldnotseethis");
    printf("File test1 has been opened with fd %d\n", fd);
    printf("The inode of the file is %d\n\n", fd_table[fd]);

    char buffer[100];
    tfs_readByte(fd, buffer);

    fd = tfs_openFile("test2");
    printf("File test2 has been opened with fd %d\n", fd);
    printf("The inode of the file is %d\n\n", fd_table[fd]);

    fd = tfs_openFile("test3");
    printf("File test3 has been opened with fd %d\n", fd);
    printf("The inode of the file is %d\n\n", fd_table[fd]);

    /* Make this file */
    fd = tfs_openFile("test4");
    printf("File test4 has been opened with fd %d\n", fd);
    printf("The inode of the file is %d\n\n", fd_table[fd]);

    if(tfs_closeFile(fd) < 0) {
        printf("Closing file failed\n");
        return -1;
    }

    fd = tfs_openFile("test5");
    printf("File test5 has been opened with fd %d\n", fd);
    printf("The inode of the file is %d\n\n", fd_table[fd]);

    fd = tfs_openFile("test6");
    printf("File test6 has been opened with fd %d\n", fd);
    printf("The inode of the file is %d\n\n", fd_table[fd]);

    return 0;
}