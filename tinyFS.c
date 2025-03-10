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
int     _parse_path(char* path, int index, char* buffer);
int     _navigate_to_dir(char* dirName, char* last_path_h, int* current_h, int* parent_h, int searching_for); 
int     _print_directory_contents(int block, int tabs);

/* error status holder */
int ERR = 0;

int tfs_mkfs(char *filename, int nBytes) {
    /* error if given 0 bytes */
    if(nBytes == 0 || filename == NULL || strlen(filename) == 0) {
        return ERR_INVALID_INPUT;
    }

    /* find how many blocks the file will use and ensure that the disk size is not too large */
    int number_of_blocks = nBytes / BLOCKSIZE;
    if(number_of_blocks > MAX_BLOCKS) {
        return ERR_INVALID_INPUT;
    }

    /* open the disk */
    int disk_descriptor = openDisk(filename, nBytes);
    if(disk_descriptor < 0) {
        return disk_descriptor;
    }

    /* buffer to write data to newly opened disk*/
    uint8_t buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE);

    /* initialize free blocks */
    buffer[BLOCK_TYPE_LOC] = FREE;
    buffer[SAFETY_BYTE_LOC] = SAFETY_HEX;
    buffer[FREE_PTR_LOC] = 0x01;
    for(int i = 1; i < number_of_blocks; i++) {

        /* if not at the last block, link to the next free block */
        if(i+1 != number_of_blocks) {
            buffer[FREE_PTR_LOC]++;

        /* if at the last block, store the last free block */
        } else {
            buffer[FREE_PTR_LOC] = 0x00;
        }

        /* update the block with the correct type, safety, and pointer bytes */
        if ((ERR = writeBlock(disk_descriptor, i, buffer)) < 0) {  
            return ERR;
        }
    }

    /* Initialize superblock and update it in the disk*/
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCK_TYPE_LOC] = SUPERBLOCK;
    buffer[SAFETY_BYTE_LOC] = SAFETY_HEX;
    buffer[FREE_PTR_LOC] = number_of_blocks > 1 ? 0x01 : 0;

    if ((ERR = writeBlock(disk_descriptor, SUPERBLOCK_DISKLOC, buffer)) < 0) {
        return ERR;
    }

    return TFS_SUCCESS;
}

int _check_data_block_consistency(int diskNum, int block, int, block_type) {
    if ((ERR = readBlock(diskNum, i, buffer)) < 0) {
        return ERR;
    }

    uint8_t byte0 = buffer[BLOCK_TYPE_LOC];
    uint8_t byte1 = buffer[SAFETY_BYTE_LOC];
    uint8_t byte2 = buffer[FREE_PTR_LOC];
    uint8_t byte3 = buffer[EMPTY_BYTE_LOC];

    if (block_type == INODE) {
        /* check the first four bytes */
        if (byte0 != INODE || byte1 != SAFETY_HEX || byte2 != EMPTY_TABLEVAL || byte3 != EMPTY_TABLEVAL) {
            return ERR_BAD_DISK;
        }

        /* check that the file type flag is valid */
        if (buffer[FILE_TYPE_FLAG_LOC] != FILE_TYPE_DIR || buffer[FILE_TYPE_FLAG_LOC] != FILE_TYPE_FLAG) {
            return ERR_BAD_DISK;
        }

        /* check that the name is valid */
        char* filename = buffer[FILE_NAME_LOC];
        if (buffer[FILE_NAME_LOC + FILENAME_LENGTH] != 0 || strlen(filename) <= 0) {
            return ERR_BAD_DISK;
        }
    }

    if (block_type == FILEEX) {
        /* check the first four bytes */
        if (byte0 != FILEEX || byte1 != SAFETY_HEX || byte2 != EMPTY_TABLEVAL || byte3 != EMPTY_TABLEVAL) {
            return ERR_BAD_DISK;
        }
    }

    if (block_type == FREE) {
        /* check the first four bytes */
        if (byte0 != FREE || byte1 != SAFETY_HEX || byte2 != EMPTY_TABLEVAL || byte3 != EMPTY_TABLEVAL) {
            return ERR_BAD_DISK;
        }

        /* make sure all remaining bytes are blank */
        for (int i = 0; i < MAX_DATA_SPACE; i++) {
            if (buffer[i + FILE_DATA_LOC] != 0) {
                return ERR_BAD_DISK;
            }
        }
    }
    
    return TFS_SUCCESS;
}

int tfs_mount(char* diskname) {
    /* make sure diskname is valid */
    if (diskname == NULL) {
        return ERR_INVALID_INPUT;
    }
    /* open the given disk */
    int diskNum = openDisk(diskname, 0);
    if (diskNum < 0) {
        return diskNum;
    }

    /* Returning an ERRor if the file isn't formatted properly */
    /* Done by making sure byte 1 of the superblock is 0x44 */
    // SYDNOTE: do we need to check the 0x44 and block type for each block in the disk file?
    // + make sure the disk file is perfectly divisible by blocksize ..? or nah
    uint8_t buffer[BLOCKSIZE];
    int i = 0;
    while (readBlock(diskNum, i, buffer) != SYS_ERR_READ) {

        uint8_t byte0 = buffer[BLOCK_TYPE_LOC];
        uint8_t byte1 = buffer[SAFETY_BYTE_LOC];
        uint8_t byte2 = buffer[FREE_PTR_LOC];

        /* check the superblock */
        if (i == 0) {
            uint8_t byte0 = buffer[BLOCK_TYPE_LOC];
            uint8_t byte1 = buffer[SAFETY_BYTE_LOC];
            if (byte0 != SUPERBLOCK || byte1 != SAFETY_HEX) {
                return ERR_BAD_DISK;
            }
        }
        i++;
    }
    
    uint8_t byte0 = buffer[BLOCK_TYPE_LOC];
    uint8_t byte1 = buffer[SAFETY_BYTE_LOC];
    if (byte0 != SUPERBLOCK || byte1 != SAFETY_HEX) {
        return ERR_BAD_DISK;
    }

    /* if there currently is a disk file mounted, unmount it */
    if (mounted != NULL) {
        tfs_unmount();
    }

    /* Initialize a new tinyFS object */
    if ((mounted = (tinyFS *) malloc(sizeof(tinyFS))) == NULL) {
        return SYS_ERR_MALLOC;
    }
    mounted->name = diskname;
    mounted->diskNum = diskNum;

    /* reset the fd table */
    memset(fd_table, 0, FD_TABLESIZE);
    // TODO: run closedisk
    return TFS_SUCCESS;
}

int tfs_unmount() {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* Free the mounted variable and change it to a null pointer */
    int returnVal = closeDisk(mounted->diskNum);

    free(mounted);
    mounted = NULL;

    //TODO: close any open files in the FD table

    /* TODO: Make sure the file is unmounted "cleanly" */
    return returnVal;
}

fileDescriptor tfs_openFile(char *name) {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* make sure the input is vlaid */
    if(name == NULL) {
        return ERR_INVALID_INPUT;
    }

    int parent = SUPERBLOCK_DISKLOC;
    char cur_path[FILENAME_LENGTH + 1];
    
    /* see if the file in the path already exists */
    int dir_found_flag = _navigate_to_dir(name, cur_path, &parent, NULL, FILE_TYPE_FILE);
    if(dir_found_flag < 0) {
        return dir_found_flag;
    }

    /* if the file already exists */
    if (dir_found_flag) {
        int fd_table_index = _update_fd_table_index();
        fd_table[fd_table_index] = parent;
        return fd_table_index;
    } 

    /* get next free block */
    uint8_t next_free_block = _pop_free_block();
    if(!next_free_block) {
        return ERR_DISK_OUT_OF_SPACE;
    }
    
    /* find the next available fd and set it to the next */ 
    if ((ERR = _update_fd_table_index()) < 0) {
        return ERR;
    }
    fd_table[fd_table_index] = next_free_block;

    /* put name of file on the inode and information bytes */
    char inode_buffer[BLOCKSIZE];
    memset(inode_buffer, 0, BLOCKSIZE);
    inode_buffer[BLOCK_TYPE_LOC] = INODE;
    inode_buffer[SAFETY_BYTE_LOC] = SAFETY_HEX;
    inode_buffer[FILE_TYPE_FLAG_LOC] = FILE_TYPE_FILE;
    int z = 0;
    while(cur_path[z] != '\000') {
        inode_buffer[z+FILE_NAME_LOC] = cur_path[z]; 
        z++;
    }

    /* turn the free block into an inode */
    if ((ERR = writeBlock(mounted->diskNum, next_free_block, inode_buffer)) < 0) {
        return ERR;
    }

    /* Get the parent */
    uint8_t parent_block[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, parent, parent_block)) < 0) {
        return ERR;
    }

    /* find the first empty byte in the superblock and add the inode */
    /* set the bounds for i based on wether in the superblock or a directory inode */
    int start_bound = parent == 0 ? FIRST_SUPBLOCK_INODE_LOC : DIR_DATA_LOC;
    int end_bound = parent == 0 ? MAX_SUPBLOCK_INODES : MAX_DIR_INODES;
    for(int i = start_bound; i < end_bound; i++) {
        if(!parent_block[i]) {
            parent_block[i] = next_free_block;
            break;
        }
    }

    /* update the parent of the file */
    if ((ERR = writeBlock(mounted->diskNum, parent, parent_block)) < 0) {
        return ERR;
    }

    return fd_table_index;
}

int tfs_closeFile(fileDescriptor FD) {
    /* make sure there is an fd entry */
    if(!fd_table[FD]) {
        return ERR_INVALID_FD;
    }

    /* remove the entry */
    fd_table[FD] = EMPTY_TABLEVAL;
    return TFS_SUCCESS;
}

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire
file’s content, to the file system. Previous content (if any) will be
completely lost. Sets the file pointer to 0 (the start of file) when
done. Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* make sure there is an fd entry */
    if(!fd_table[FD]) {
        return ERR_INVALID_FD;
    }

    /* Grab the block's inode */
    uint8_t inode[BLOCKSIZE]; 
    if ((ERR = readBlock(mounted->diskNum, fd_table[FD], inode)) < 0) {
        return ERR;
    }

    /* store the file size in the inode */
    int i = FILE_SIZE_LOC;
    inode[i] = (size >> 24) & 0xFF;
    inode[i + 1] = (size >> 16) & 0xFF;
    inode[i + 2] = (size >> 8) & 0xFF;
    inode[i + 3] = size & 0xFF;

    // Resetting the direct blocks so the data is "lost" since nothing is pointing to them
    // Counter for clearing
    i = 0;
    uint8_t dir_block = inode[FILE_DATA_LOC + i++];
    while(dir_block != 0x0) {
        // Allocating each block as "free" and adding them to the linked list
        if ((ERR = _free_block(dir_block)) < 0) {
            return ERR;
        }

        dir_block = inode[FILE_DATA_LOC + i++];
    }

    // Determining the amount of blocks to be written. A plus one at the end for data outside the 256 byte margin.
    // NOTE TO PROGRAMMER: I set this to size-1 so write of 256 bytes(or any number on the line)
    // will not take up extra blocks. Might cause problems in the future.
    int numBlocks = ((size-1) / MAX_DATA_SPACE) + 1;
    // Writing those blocks to the file
    uint8_t temp_addr;
    uint8_t temp_block[BLOCKSIZE];
    for (int i = 0; i < numBlocks; i++) {
        // A value to keep track of where we are in the buffer
        int bufferHead = 0;
        // A variable to keep track of how many bytes should be written so that bytes outside the buffer aren't included
        int writeSize = size - (i * MAX_DATA_SPACE);
        temp_addr = _pop_free_block();
        if (!temp_addr) {
            return ERR_DISK_OUT_OF_SPACE;
        }
        if ((ERR = readBlock(mounted->diskNum, temp_addr, temp_block)) < 0) {
            return ERR;
        }
        // Update important data
        temp_block[BLOCK_TYPE_LOC] = FILEEX;

        // Copy the buffer data over to the block
        if(writeSize > MAX_DATA_SPACE) {
            writeSize = MAX_DATA_SPACE;
        }
        for (int j = FIRST_DATA_LOC; j < writeSize + FIRST_DATA_LOC; j++) {
            temp_block[j] = buffer[bufferHead++];
        }
        if ((ERR = writeBlock(mounted->diskNum, temp_addr, temp_block)) < 0) {
            return ERR;
        }

        /* update the file inode with the data block */
        inode[FILE_DATA_LOC + i] = temp_addr;
        //SYDNOTE: moved this into the for loop, is it supposed to be out of it?
        if ((ERR = writeBlock(mounted->diskNum, fd_table[FD], inode)) < 0) {
            return ERR;
        }
    }

    /* set the file offset to be 0 */
    if ((ERR = tfs_seek(FD, 0)) < 0) {
        return ERR;
    }

    return TFS_SUCCESS;
}

int tfs_deleteFile(fileDescriptor FD) {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* make sure there is an fd entry */
    if(!fd_table[FD]) {
        return ERR_INVALID_FD;
    }

    /* Grab the block's inode */
    uint8_t inode[BLOCKSIZE]; 
    if ((ERR = readBlock(mounted->diskNum, fd_table[FD], inode)) < 0 ) {
        return ERR;
    }

    // Resetting the direct blocks so the data is "lost" since nothing is pointing to them
    // Counter for clearing
    int i = 0;
    uint8_t dir_block = inode[FILE_DATA_LOC + i++];
    while(dir_block != 0x0) {
        // Allocating each block as "free" and adding them to the linked list
        if ((ERR = _free_block(dir_block)) < 0) {
            return ERR;
        }
        dir_block = inode[FILE_DATA_LOC + i++];
    }

    /* Finally, freeing the inode */
    if ((ERR = _free_block(fd_table[FD])) < 0) {
        return ERR;
    }

    /* close the file in the fd table and if there are 
       multiple FDs for the file, close those too */
    int node = fd_table[FD];
    for(int i = 0; i < FD_TABLESIZE; i++) {
        if(fd_table[i] == node) {
            if ((ERR = tfs_closeFile(i)) < 0) {
                return ERR;
            }
        }
    }

    return TFS_SUCCESS;
}

int tfs_readByte(fileDescriptor FD, char* buffer) {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* make sure there is an fd entry */
    if(!fd_table[FD]) {
        return ERR_INVALID_FD;
    }

    /* grab the inode block */
    uint8_t inode[BLOCKSIZE]; 
    if ((ERR = readBlock(mounted->diskNum, fd_table[FD], inode)) < 0) {
        return ERR;
    }

    /* get file offset from inode block and convert to block & block offset */
    int i = FILE_OFFSET_LOC;
    int offset = (inode[i] << 24) + (inode[i + 1] << 16) + (inode[i + 2] << 8) + inode[i + 3];
    int block_num = offset / MAX_DATA_SPACE;
    int block_offset = offset % MAX_DATA_SPACE;

    i = FILE_SIZE_LOC;
    int size = (inode[i] << 24) + (inode[i + 1] << 16) + (inode[i + 2] << 8) + inode[i + 3];

    /* make sure that the offset is not outside of the file */
    if(offset > size) {
        return ERR_FILE_PNTR_OUT_OF_BOUNDS;
    }

    /* increment the offset and make sure it is in the file */
    if ((ERR = tfs_seek(FD, ++offset)) < 0) {
        return ERR;
    }

    /* grab the data block */
    uint8_t data_block_num = inode[FILE_DATA_LOC + block_num];
    uint8_t data_block[BLOCKSIZE]; 
    if ((ERR = readBlock(mounted->diskNum, data_block_num, data_block)) < 0) {
        return ERR;
    }

    /* store the byte at the offset into the given buffer */
    buffer[0] =  data_block[FIRST_DATA_LOC + block_offset];
    
    return TFS_SUCCESS;
}

int tfs_seek(fileDescriptor FD, int offset) {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* make sure there is an fd entry */
    if(!fd_table[FD]) {
        return ERR_INVALID_FD;
    }

    if (offset < 0) {
        return ERR_INVALID_INPUT;
    }

    /* grab the inode */
    uint8_t inode[BLOCKSIZE]; 
    if ((ERR = readBlock(mounted->diskNum, fd_table[FD], inode)) < 0) {
        return ERR;
    }

    /* get the file size */
    int i = FILE_SIZE_LOC;
    int size = (inode[i] << 24) + (inode[i + 1] << 16) + (inode[i + 2] << 8) + inode[i + 3];

    /* make sure the offset is in the file */
    if (offset > size) {
        return ERR_FILE_PNTR_OUT_OF_BOUNDS;
    } 

    /* store the offset in the file inode */
    i = FILE_OFFSET_LOC;
    inode[i] = (offset >> 24) & 0xFF;
    inode[i + 1] = (offset >> 16) & 0xFF;
    inode[i + 2] = (offset >> 8) & 0xFF;
    inode[i + 3] = offset & 0xFF;

    if ((ERR = writeBlock(mounted->diskNum, fd_table[FD], inode)) < 0) {
        return ERR;
    }

    return TFS_SUCCESS;
}

/* ~ ADDITIONAL FEATURES ~ */

/* (C) hierarchical directories */

// return dir_found_flag
int _navigate_to_dir(char* dirName, char* last_path_h, int* current_h, int* parent_h, int searching_for) {

    /* The superblock effectively behaves as the inode for the root */
    int current = SUPERBLOCK_DISKLOC;
    uint8_t current_block[BLOCKSIZE]; 
    int parent = current;
    if ((ERR = readBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, current_block)) < 0) {
        return ERR;
    }

    bool dir_found_flag = false; 
    int path_index = 0;
    char cur_path[FILENAME_LENGTH + 1]; 
    char inode_buffer[BLOCKSIZE];

    /* navigate through the directories until the end */
    while (path_index != strlen(dirName) + 1) {

        /* parse thru dirName to get the next path name and make sure valid */
        path_index = _parse_path(dirName, path_index, cur_path);
        if (path_index < 0) {
            return path_index;
        }

        /* look through the inode pointers in the parent_block for the next path */
        dir_found_flag = false;

        /* set the bounds for i based on wether in the superblock or a directory inode */
        int start_bound = current == 0 ? FIRST_SUPBLOCK_INODE_LOC : DIR_DATA_LOC;
        int end_bound = current == 0 ? MAX_SUPBLOCK_INODES : MAX_DIR_INODES;
        for(int i = start_bound; i < end_bound; i++) {

            /* skip over if we have an empty block, meaning no inode exists there */
            if(!current_block[i]) {
                continue;
            }

            /* Grab the name from the inode buffer */
            memset(inode_buffer, 0, BLOCKSIZE);
            if ((ERR = readBlock(mounted->diskNum, current_block[i], inode_buffer)) < 0) {
                return ERR;
            }
            char* filename = inode_buffer + FILE_NAME_LOC; 

            /* if found, reset that directory as the parent_block */
            if(strcmp(cur_path, filename) == 0) {
                dir_found_flag = true;

                /* make sure the file found is of type 'directory' */
                if (inode_buffer[FILE_TYPE_FLAG_LOC] == FILE_TYPE_FILE && (path_index != strlen(dirName) + 1 || searching_for == FILE_TYPE_DIR)) {
                    return ERR_NOT_A_DIR;
                }

                /* get the inode of the found directory and store it as the parent */
                parent = current;
                current = current_block[i];
                if ((ERR = readBlock(mounted->diskNum, current_block[i], current_block)) < 0) {
                    return ERR;
                }
                
                break;
            }
        }

        /* if not at the last path and unable to find the directory, error */
        if (path_index != strlen(dirName) + 1 && !dir_found_flag) {
            return ERR_DIR_NOT_FOUND; // ERR: not a directory, directory not found
        }

    }

    if (current_h != NULL) {
        *current_h = current;
    }
    if (parent_h != NULL) {
        *parent_h = parent;
    }
    strcpy(last_path_h, cur_path);

    return dir_found_flag;
}

/* creates a directory, name could contain a “/”-delimited path) */
int tfs_createDir(char* dirName) {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* make sure the given dirName is not null */
    if (dirName == NULL) {
        return ERR_INVALID_INPUT;  // ERR: invalid input error
    }

    int parent = SUPERBLOCK_DISKLOC;
    char cur_path[FILENAME_LENGTH + 1];
    
    /* make sure the last directory in the path does not already exist */
    int dir_found_flag = _navigate_to_dir(dirName, cur_path, &parent, NULL, FILE_TYPE_DIR);
    if(dir_found_flag < 0) {
        return dir_found_flag;
    }

    if (dir_found_flag) {
        // dir already exists 
        return ERR_DIR_ALREADY_EXISTS;
    }

    /* get the next free block to store the new inode in */
    uint8_t next_free_block = _pop_free_block();
    if(!next_free_block) {
        return ERR_DISK_OUT_OF_SPACE;
    }

    /* set up the new inode: put name of file on the inode and information bytes */
    char inode_buffer[BLOCKSIZE];
    memset(inode_buffer, 0, BLOCKSIZE);
    inode_buffer[BLOCK_TYPE_LOC] = INODE;
    inode_buffer[SAFETY_BYTE_LOC] = SAFETY_HEX;
    inode_buffer[FILE_TYPE_FLAG_LOC] = FILE_TYPE_DIR;
    int z = 0;
    while(cur_path[z] != '\000') {
        inode_buffer[FILE_NAME_LOC + z] = cur_path[z];
        z++;
    }

    /* write the inode into the grabbed free_block */
    if ((ERR = writeBlock(mounted->diskNum, next_free_block, inode_buffer)) < 0) {
        return ERR;
    }

    /* grab the parent's inode and update its pointers to hold the new directory */
    uint8_t parent_block[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, parent, parent_block)) < 0) {
        return ERR;
    }

    /* find the first empty byte in the parent block and add the inode */
    /* set the bounds for i based on wether in the superblock or a directory inode */
    int start_bound = parent == 0 ? FIRST_SUPBLOCK_INODE_LOC : DIR_DATA_LOC;
    int end_bound = parent == 0 ? MAX_SUPBLOCK_INODES : MAX_DIR_INODES;
    for(int i = start_bound; i < end_bound; i++) {
        if(!parent_block[i]) {
            parent_block[i] = next_free_block;
            break;
        }
    }

    /* update the parent block */
    if ((ERR = writeBlock(mounted->diskNum, parent, parent_block)) < 0) {
        return ERR;
    }

    return TFS_SUCCESS;
} 

/* deletes empty directory */
int tfs_removeDir(char* dirName) {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* make sure the given dirName is not null */
    if (dirName == NULL) {
        return ERR_INVALID_INPUT;  // ERR: invalid input error
    }

    int current = SUPERBLOCK_DISKLOC;
    int parent = current;
    char cur_path[FILENAME_LENGTH + 1];
    
    /* make sure the last directory in the path does already exist */
    int dir_found_flag = _navigate_to_dir(dirName, cur_path, &current, &parent, FILE_TYPE_DIR);
    if(dir_found_flag < 0) {
        return dir_found_flag;
    }

    if (!dir_found_flag) {
        // trying to remove a directory that does not exist
        return ERR_DIR_NOT_FOUND; 
    }

    /* re-grab the block of the directory and make sure it is empty */
    char inode_buffer[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, current, inode_buffer)) < 0) {
        return ERR;
    }
    for(int i = DIR_DATA_LOC; i < MAX_DIR_INODES; i++) {
        if(inode_buffer[i]) {
            return ERR_DIR_NOT_EMPTY; // ERR: directory is not empty
        }
    }

    /* remove the directory inode block and update its parent indoe */
    if ((ERR = _free_block(current)) < 0) {
        return ERR;
    }
    uint8_t parent_block[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, parent, parent_block)) < 0) {
        return ERR;
    }

    /* set the bounds for i based on wether in the superblock or a directory inode */
    int start_bound = parent == 0 ? FIRST_SUPBLOCK_INODE_LOC : DIR_DATA_LOC;
    int end_bound = parent == 0 ? MAX_SUPBLOCK_INODES : MAX_DIR_INODES;
    for(int i = start_bound; i < end_bound; i++) {
        if(parent_block[i] == current) {
            parent_block[i] = EMPTY_TABLEVAL;
        }
    }

    /* update the parent block */
    if ((ERR = writeBlock(mounted->diskNum, parent, parent_block)) < 0) {
        return ERR;
    }

    return TFS_SUCCESS;
}

/* recursively remove dirName and any file and directories under it. 
Special “/” token may be used to indicate root dir. */
int tfs_removeAll(char* dirName) {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* make sure the given dirName is not null */
    if (dirName == NULL) {
        return ERR_INVALID_INPUT;  // ERR: invalid input error
    }

    int current = SUPERBLOCK_DISKLOC;
    int parent = current;
    char cur_path[FILENAME_LENGTH + 1];
    
    /* make sure the last directory in the path does already exist */
    /* skip if given dirName "/" */
    if (strcmp(dirName, "/") != 0) {
        int dir_found_flag = _navigate_to_dir(dirName, cur_path, &current, &parent, FILE_TYPE_DIR);
        if(dir_found_flag < 0) {
            return dir_found_flag;
        }

        if (!dir_found_flag) {
            // trying to remove a directory that does not exist
            return ERR_DIR_NOT_FOUND; // ERR: not a directory, directory not found
        }
    }
    
    /* re-grab the block of the directory and remove every item in it */
    uint8_t current_inode[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, current, current_inode)) < 0) {
        return ERR;
    }
    char inode_buffer[BLOCKSIZE];

    /* set the bounds for i based on wether in the superblock or a directory inode */
    int start_bound = current == 0 ? FIRST_SUPBLOCK_INODE_LOC : DIR_DATA_LOC;
    int end_bound = current == 0 ? MAX_SUPBLOCK_INODES : MAX_DIR_INODES;
    for(int i = start_bound; i < end_bound; i++) {
        if(current_inode[i]) {
            memset(inode_buffer, 0, BLOCKSIZE);
            readBlock(mounted->diskNum, current_inode[i], inode_buffer);

            if (inode_buffer[FILE_TYPE_FLAG_LOC] == FILE_TYPE_FILE) {
                /* SYDNOTE: code copied from deleteFile, should make a helper function
                called smth like deleteFileByInode where all the same code except 
                replace fd_table[FD] with the inode block number */
                // Grab the block's inode
                // TODO : make sure matches w deleteFile after done fixing things
                uint8_t inode[BLOCKSIZE]; 
                if ((ERR = readBlock(mounted->diskNum, current_inode[i], inode)) < 0) {
                    return ERR;
                }

                // Resetting the direct blocks so the data is "lost" since nothing is pointing to them
                // Counter for clearing
                int i = 0;
                uint8_t data_block = inode[FILE_DATA_LOC + i++];
                while(data_block != 0x0) {
                    // Allocating each block as "free" and adding them to the linked list
                    if ((ERR = _free_block(data_block)) < 0) {
                        return ERR;
                    }
                    data_block = inode[FILE_DATA_LOC + i++];
                }

                // Finally, freeing the inode
                if ((ERR = _free_block(current_inode[i])) < 0) {
                    return ERR;
                }
            } else if (inode_buffer[FILE_TYPE_FLAG_LOC] == FILE_TYPE_DIR) {
                uint8_t inode[BLOCKSIZE]; 
                if ((ERR = readBlock(mounted->diskNum, current_inode[i], inode)) < 0) {
                    return ERR;
                }

                char* dir_path = malloc(sizeof(dirName) + sizeof(inode[FILE_NAME_LOC]) + 1);
                if (dir_path == NULL) {
                    return SYS_ERR_MALLOC;
                }
                strcpy(dir_path, dirName);
                strcat(dir_path, "/");
                strcat(dir_path, inode_buffer + FILE_NAME_LOC);

                if ((ERR = tfs_removeAll(dir_path)) < 0) { 
                    return ERR;
                }
                /* SYDNOTE: hmm maybe we do need a helper where removeall just calls a 
                helper to remove all by given dir inode rather than dirname 
                then we can use that to minimiz duplication between createDir, removeDir, 
                removeAll, and then use it here to recurse */
                free(dir_path); // TODO: test that this works
            }
        }
    }

    /* if not the root directory,
    once everything is removed, delete the current directory */
    return parent == 0 ? 0 : tfs_removeDir(dirName);
}

/* (B) directory listing and file renaming */

/* renames a file. New name should be passed in. File has to be open. */
int tfs_rename(fileDescriptor FD, char* newName) {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* make sure there is an fd entry */
    if(!fd_table[FD]) {
        return ERR_INVALID_FD;
    }

    /* make sure the given inputs are valid */
    if (newName == NULL || FD <= 0 || strlen(newName) > 8) {
        return ERR_INVALID_INPUT; // ERR: invalid input
    }

    // TODO (?): check if newName already exists -> helper func fs

    /* read in the inode corresponding to the given fd */
    char inode[BLOCKSIZE]; 
    if ((ERR = readBlock(mounted->diskNum, fd_table[FD], inode)) < 0) {
        return ERR;
    }

    /* clear out the current inode's name and write in the new one */
    char* filename = inode + FILE_NAME_LOC;
    memset(filename, 0, FILENAME_LENGTH);

    int z = 0;
    while(newName[z] != '\000' && z < FILENAME_LENGTH) {
        inode[FILE_NAME_LOC + z] = newName[z];
        z++;
    }
    inode[FILE_NAME_LOC + z] = '\0';

    /* update the inode */
    if ((ERR = writeBlock(mounted->diskNum, fd_table[FD], inode)) < 0) {
        return ERR;
    }

    return TFS_SUCCESS;
}

/* lists all the files and directories on the disk, print the list to stdout */
int tfs_readdir() {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    uint8_t superblock[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock)) < 0) {
        return ERR;
    }

    uint8_t inode[BLOCKSIZE];
    for(int i = 0; i < MAX_SUPBLOCK_INODES; i++) {

        if(superblock[i + FIRST_SUPBLOCK_INODE_LOC]) {

            if ((ERR = readBlock(mounted->diskNum, superblock[i + FIRST_SUPBLOCK_INODE_LOC], inode)) < 0) {
                return ERR;
            }
    
            if(inode[FILE_TYPE_FLAG_LOC] == FILE_TYPE_DIR) {
                printf("%s\n", (inode + FILE_NAME_LOC));
                if ((ERR = _print_directory_contents(superblock[i + FIRST_SUPBLOCK_INODE_LOC], 1)) < 0) {
                    return ERR;
                }
            } else {
                printf("%s\n", (inode + FILE_NAME_LOC));
            }

        }
    }
    return TFS_SUCCESS;
}

/* (E) timestamps */
/* returns the file’s creation time or all info */
int tfs_readFileInfo(fileDescriptor FD) {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* make sure there is an fd entry */
    if(!fd_table[FD]) {
        return ERR_INVALID_FD;
    }

    // TODO

    return TFS_SUCCESS;
} 

/* ~ HELPER FUNCTIONS ~ */
// SYDNOTE: make sure we are error handling the helper functions when using them


/* _parse_path(): parse the given path to get the next directory/file at the given index
    + store the path name in the given buffer, up to the max filename length
    > return the index of where the next path name starts
    > returns the length of the path + 1 if the current path name is the end of the path
    > return < 0 if errors
        - error if the next path name is more than the max filename length
        - error if the path name has un-readable characters */
int _parse_path(char* path, int index, char* buffer) {

    if(path[index] == '\0') {
        return ERR_INVALID_INPUT;
    }
    /* stop at '\0' or '/' or until up to the allowed filename length */
    int i = 0;
    while (path[index] != '\0' && path[index] != '/' && i < FILENAME_LENGTH) {
        /* make sure the chars are valid, readable chars */
        if (path[index] < 0x20) {
            return ERR_INVALID_INPUT;  // ERR; invalid input, char not a readable character
        }

        buffer[i++] = path[index++];
    }
    buffer[i] = '\0';

    /* make sure the last char is the end of the name */
    if (path[index] != '\0' && path[index] != '/') {
        return ERR_INVALID_INPUT;
    }

    return ++index;
}

/* get the the next available fd table index */
int _update_fd_table_index() {
    int original = fd_table_index;

    /* iterate to the end of the list until next available fd */
    for(int i = fd_table_index; i < FD_TABLESIZE; i++) {
        if(fd_table[fd_table_index] == EMPTY_TABLEVAL) {
            return fd_table_index;
        } else {
            fd_table_index++;
        }
    }

    /* Start over until up until original starting point */
    fd_table_index = 0;
    for(int i=0; i < original; i++) {
        if(fd_table[fd_table_index] == EMPTY_TABLEVAL) {
            return fd_table_index;
        } else {
            fd_table_index++;
        }
    }

    /* if you made it this far, there are no available file descriptors */
    return ERR_OUT_OF_FDS;
}

/* Pop and return the next free block, and replace the superblock index
 with that block's next block. Should return 0 if no more free blocks exist. */
uint8_t _pop_free_block() {
    // Grab the superblock. This is done locally as some functions may not
    // need to store the superblock so this function does it just in case
    uint8_t superblock[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock)) < 0) {
        return ERR;
    }

    uint8_t newBlock[BLOCKSIZE];
    uint8_t next_free_block = superblock[FREE_PTR_LOC];
    /* then, grab the address for the next free inode*/
    if ((ERR = readBlock(mounted->diskNum, next_free_block, newBlock)) < 0) {
        return ERR;
    }
    /* update next free block */
    superblock[FREE_PTR_LOC] = newBlock[FREE_PTR_LOC];
    if ((ERR = writeBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock)) < 0) {
        return ERR;
    }
    return next_free_block;
}

/* free_block turns the given block into a free block, and also adds
    it to the list of free blocks. Returns a 0 on success or -1 on error*/
int _free_block(uint8_t block_addr) {
    // Grab the superblock
    uint8_t superblock[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock)) < 0) {
        return ERR;
    }

    // Change block type
    uint8_t clean_block[BLOCKSIZE];
    clean_block[BLOCK_TYPE_LOC] = FREE;
    clean_block[SAFETY_BYTE_LOC] = SAFETY_HEX;
    clean_block[FREE_PTR_LOC] = superblock[FREE_PTR_LOC];
    memset(clean_block + FIRST_DATA_LOC, 0, MAX_DATA_SPACE);
    if ((ERR = writeBlock(mounted->diskNum, block_addr, clean_block)) < 0) {
        return ERR;
    }
    
    // Change free list
    superblock[FREE_PTR_LOC] = block_addr;
    if ((ERR = writeBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock)) < 0) {
        return ERR;
    }

    return TFS_SUCCESS;
}

int _print_directory_contents(int block, int tabs) {

    uint8_t directory_inode[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, block, directory_inode)) < 0) {
        return ERR;
    }

    uint8_t inode[BLOCKSIZE];
    for(int i = 0; i < MAX_DIR_INODES; i++) {

        if(directory_inode[i + DIR_DATA_LOC]) {

            if ((ERR = readBlock(mounted->diskNum, directory_inode[i + DIR_DATA_LOC], inode)) < 0) {
                return ERR;
            }

            for(int i = 0; i < tabs; i++) {
                printf("     ");
            }
            if(inode[FILE_TYPE_FLAG_LOC] == FILE_TYPE_DIR) {
                printf("%s\n", (inode + FILE_NAME_LOC));
                if ((ERR = _print_directory_contents(directory_inode[i + DIR_DATA_LOC], tabs+1)) < 0) {
                    return ERR;
                }
            } else {
                printf("%s\n", (inode + FILE_NAME_LOC));
            }

        }
    }

    return TFS_SUCCESS;
}

/* Uncomment and run this block if you want to test */
int main(int argc, char* argv[]) {
    int err;
    if(argc > 1) {
        err = tfs_mkfs("SydneysDisk.dsk", 512);   
        printf("err: %d\n", err); 
    }


    if((err = tfs_mount("SydneysDisk.dsk")) < 0) {
        printf("Failed to mount Sydney's disk: %d\n", err);
        return -1;
    }


    printf("%d\n", tfs_createDir("testDir"));

//     printf("%d\n", tfs_createDir("testDir/quincys"));

//     printf("%d\n", tfs_createDir("testDir/quincys/anisdir"));

//     printf("%d\n", tfs_createDir("testDir/quincys/anisdir/ARAV"));

//     printf("%d\n", tfs_createDir("testDir/mars"));

//     printf("%d\n", tfs_openFile("testDir/quincys/whee"));

//     printf("%d\n", tfs_openFile("testDir/quincys/whoo"));

//     printf("%d\n", tfs_openFile("testDir/quincys/anisdir/HAHA"));

//     printf("%d\n", tfs_openFile("testDir/quincys/anisdir/ARAV/fuck"));

//     printf("%d\n", tfs_openFile("testDir/quincys/anisdir/ARAV/shit"));

//     printf("%d\n", tfs_openFile("testDir/quincys/anisdir/ARAV/piss"));


//     tfs_readdir();

// //     printf("%d\n", tfs_removeDir("testDir/quincys/anisdir/ARAV"));

// //     printf("%d\n", tfs_removeAll("testDir/quincys"));


    return 0;
}