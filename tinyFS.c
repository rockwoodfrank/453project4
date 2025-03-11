#include "tinyFS.h"
#include "libTinyFS.h"

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
    char buffer[BLOCKSIZE];
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

int tfs_mount(char* diskname) {
    /* make sure diskname is valid */
    if (diskname == NULL) {
        return ERR_INVALID_INPUT;
    }

    /* if there currently is a disk file mounted, unmount it */
    if (mounted != NULL) {
        tfs_unmount();
    }

    /* open the given disk */
    int diskNum = openDisk(diskname, 0);
    if (diskNum < 0) {
        return diskNum;
    }

    struct stat file_stat;
    if (fstat(diskNum, &file_stat) == -1) {
        return SYS_ERR_FSTAT;
    }  

    /* make sure nBytes is evenly divisible by BLOCKSIZE */
    if(file_stat.st_size % BLOCKSIZE != 0) {
        return ERR_BAD_DISK;
    }
    int num_blocks = file_stat.st_size / BLOCKSIZE;
    char* blocks_checked = (char*) malloc(num_blocks);
    if (blocks_checked == NULL) {
        return SYS_ERR_MALLOC;
    }
    memset(blocks_checked, 0, num_blocks);

    /* Returning an ERRor if the file isn't formatted properly */
    if (_check_block_con(diskNum, SUPERBLOCK_DISKLOC, SUPERBLOCK, blocks_checked) < 0) {
        return ERR_BAD_DISK;
    }

    /* Checks to see if every block is marked as checked */
    for (int i = 0; i < num_blocks; i++) {
        if (blocks_checked[i] == 0) {
            return ERR_BAD_DISK;
        }
    }
    free(blocks_checked);

    /* Initialize a new tinyFS object */
    if ((mounted = (tinyFS *) malloc(sizeof(tinyFS))) == NULL) {
        return SYS_ERR_MALLOC;
    }
    mounted->name = diskname;
    mounted->diskNum = diskNum;

    /* reset the fd table */
    memset(fd_table, 0, FD_TABLESIZE);
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

    memset(fd_table, 0, FD_TABLESIZE);

    return returnVal;
}

fileDescriptor tfs_openFile(char *name) {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* make sure the given dirName is not null */
    if (name == NULL) {
        return ERR_INVALID_INPUT;  // ERR: invalid input error
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
        uint8_t *inode = malloc(BLOCKSIZE * sizeof(char));
        if ((ERR = readBlock(mounted->diskNum, parent, inode)) < 0) {
            return ERR;
        }
        _write_long(inode, time(NULL), FILE_ACCESSTIME_LOC);

        
        if ((ERR = writeBlock(mounted->diskNum, parent, inode)) < 0) {
            return ERR;
        }

        free(inode);
        return fd_table_index;
    } 

    /* get next free block */
    char next_free_block = _pop_free_block();
    if(!next_free_block) {
        return ERR_DISK_OUT_OF_SPACE;
    }
    
    /* find the next available fd and set it to the next */ 
    if ((ERR = _update_fd_table_index()) < 0) {
        return ERR;
    }
    fd_table[fd_table_index] = next_free_block;

    /* put name of file on the inode and information bytes */
    uint8_t inode_buffer[BLOCKSIZE];
    memset(inode_buffer, 0, BLOCKSIZE);
    inode_buffer[BLOCK_TYPE_LOC] = INODE;
    inode_buffer[SAFETY_BYTE_LOC] = SAFETY_HEX;
    inode_buffer[FILE_TYPE_FLAG_LOC] = FILE_TYPE_FILE;
    // Writing the time, since it's 8 bytes
    _write_long(inode_buffer, time(NULL), FILE_CREATEDTIME_LOC);
    _write_long(inode_buffer, time(NULL), FILE_MODIFIEDTIME_LOC);
    _write_long(inode_buffer, time(NULL), FILE_ACCESSTIME_LOC);
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
    char parent_block[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, parent, parent_block)) < 0) {
        return ERR;
    }

    /* find the first empty byte in the superblock and add the inode */
    /* set the bounds for i based on wether in the superblock or a directory inode */
    int start_bound = parent == 0 ? FIRST_SUPBLOCK_INODE_LOC : DIR_DATA_LOC;
    int range = parent == 0 ? MAX_SUPBLOCK_INODES : MAX_DIR_INODES;
    for(int i = start_bound; i < range + start_bound; i++) {
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
    if(!fd_table[FD] || buffer == NULL) {
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
    _write_long(inode, time(NULL), FILE_MODIFIEDTIME_LOC);

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
    char temp_addr;
    char temp_block[BLOCKSIZE];
    int bufferHead = 0;
    for (int i = 0; i < numBlocks; i++) {
        // A value to keep track of where we are in the buffer
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
        temp_block[SAFETY_BYTE_LOC] = SAFETY_HEX;
        temp_block[FREE_PTR_LOC] = EMPTY_TABLEVAL;
        temp_block[EMPTY_BYTE_LOC] = EMPTY_TABLEVAL;

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
    char inode_num = fd_table[FD];

    int parent = _fetch_parent(inode_num);
    if (parent < 0) {
        return parent;
    }

    return _remove_inode_and_blocks(inode_num, parent);
}

int tfs_readByte(fileDescriptor FD, char* buffer) {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* make sure there is an fd entry */
    if(!fd_table[FD] || buffer == NULL) {
        return ERR_INVALID_FD;
    }

    /* grab the inode block */
    uint8_t inode[BLOCKSIZE]; 
    if ((ERR = readBlock(mounted->diskNum, fd_table[FD], inode)) < 0) {
        return ERR;
    }

    _write_long(inode, time(NULL), FILE_ACCESSTIME_LOC);

    /* get file offset from inode block and convert to block & block offset */
    int i = FILE_OFFSET_LOC;
    uint32_t offset = (inode[i] << 24) + (inode[i + 1] << 16) + (inode[i + 2] << 8) + inode[i + 3];
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
    char data_block_num = inode[FILE_DATA_LOC + block_num];
    char data_block[BLOCKSIZE]; 
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
    if (newName == NULL || FD <= 0 || strlen(newName) > 8 || strlen(newName) < 1) {
        return ERR_INVALID_INPUT; // ERR: invalid input
    }

    /* read in the inode corresponding to the given fd */
    char inode[BLOCKSIZE]; 
    if ((ERR = readBlock(mounted->diskNum, fd_table[FD], inode)) < 0) {
        return ERR;
    }
    _write_long((uint8_t*)inode, time(NULL), FILE_CREATEDTIME_LOC);
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

    char superblock[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock)) < 0) {
        return ERR;
    }

    char inode[BLOCKSIZE];
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

/* (C) hierarchical directories */

/* creates a directory, name could contain a “/”-delimited path) */
int tfs_createDir(char* dirName) {
    /* make sure there is a mounted tfs */
    if (mounted == NULL) {
        return ERR_NO_DISK_MOUNTED;
    }

    /* make sure the given dirName is not null */
    if (dirName == NULL || dirName[0] != '/') {
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
    char next_free_block = _pop_free_block();
    if(!next_free_block) {
        return ERR_DISK_OUT_OF_SPACE;
    }

    /* set up the new inode: put name of file on the inode and information bytes */
    uint8_t inode_buffer[BLOCKSIZE];
    memset(inode_buffer, 0, BLOCKSIZE);
    inode_buffer[BLOCK_TYPE_LOC] = INODE;
    inode_buffer[SAFETY_BYTE_LOC] = SAFETY_HEX;
    inode_buffer[FILE_TYPE_FLAG_LOC] = FILE_TYPE_DIR;
    _write_long(inode_buffer, time(NULL), FILE_CREATEDTIME_LOC);
    _write_long(inode_buffer, time(NULL), FILE_ACCESSTIME_LOC);
    _write_long(inode_buffer, time(NULL), FILE_MODIFIEDTIME_LOC);
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
    char parent_block[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, parent, parent_block)) < 0) {
        return ERR;
    }

    /* find the first empty byte in the parent block and add the inode */
    /* set the bounds for i based on wether in the superblock or a directory inode */
    int start_bound = parent == 0 ? FIRST_SUPBLOCK_INODE_LOC : DIR_DATA_LOC;
    int range = parent == 0 ? MAX_SUPBLOCK_INODES : MAX_DIR_INODES;
    for(int i = start_bound; i < range + start_bound; i++) {
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
    char parent_block[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, parent, parent_block)) < 0) {
        return ERR;
    }

    /* set the bounds for i based on wether in the superblock or a directory inode */
    int start_bound = parent == 0 ? FIRST_SUPBLOCK_INODE_LOC : DIR_DATA_LOC;
    int range = parent == 0 ? MAX_SUPBLOCK_INODES : MAX_DIR_INODES;
    for(int i = start_bound; i < range + start_bound; i++) {
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
    char current_inode[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, current, current_inode)) < 0) {
        return ERR;
    }
    char inode_buffer[BLOCKSIZE];

    /* set the bounds for i based on wether in the superblock or a directory inode */
    int start_bound = current == 0 ? FIRST_SUPBLOCK_INODE_LOC : DIR_DATA_LOC;
    int range = current == 0 ? MAX_SUPBLOCK_INODES : MAX_DIR_INODES;
    for(int i = start_bound; i < range + start_bound; i++) {
        if(current_inode[i]) {
            memset(inode_buffer, 0, BLOCKSIZE);
            readBlock(mounted->diskNum, current_inode[i], inode_buffer);

            if (inode_buffer[FILE_TYPE_FLAG_LOC] == FILE_TYPE_FILE) {
                if ((ERR = _remove_inode_and_blocks(current_inode[i], current)) < 0) {
                    return ERR;
                }
                current_inode[i] = 0x0;
                if ((ERR = writeBlock(mounted->diskNum, current, current_inode)) < 0) {
                    return ERR;
                }
            } else if (inode_buffer[FILE_TYPE_FLAG_LOC] == FILE_TYPE_DIR) {
                char inode[BLOCKSIZE]; 
                if ((ERR = readBlock(mounted->diskNum, current_inode[i], inode)) < 0) {
                    return ERR;
                }

                char* dir_path = malloc(sizeof(dirName) + sizeof(inode[FILE_NAME_LOC]) + 1);
                if (dir_path == NULL) {
                    return SYS_ERR_MALLOC;
                }

                strcpy(dir_path, dirName);
                if (strcmp(dirName, "/") != 0) {
                    strcat(dir_path, "/");
                }
                strcat(dir_path, inode_buffer + FILE_NAME_LOC);

                if ((ERR = tfs_removeAll(dir_path)) < 0) { 
                    return ERR;
                }
                free(dir_path);
            }
            current_inode[i] = 0x0;
            if ((ERR = writeBlock(mounted->diskNum, current, current_inode)) < 0) {
                return ERR;
            }
        }
    }


    /* if not the root directory,
    once everything is removed, delete the current directory */
    return current == 0 ? 0 : tfs_removeDir(dirName);
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

    time_t* createdTime;
    time_t* modifiedTime;
    time_t* accessedTime;
    uint8_t* fileName;
    int fileSize;
    uint8_t inode[BLOCKSIZE];
    
    if ((ERR = readBlock(mounted->diskNum, fd_table[FD], inode)) < 0) {
        return ERR;
    }
    fileName = inode + FILE_NAME_LOC;
    // Print file name
    printf("Name:\t\t%s\n", fileName);
    if (inode[FILE_TYPE_FLAG_LOC] == FILE_TYPE_FILE) {
        // Print file size if it's a file
        int s = FILE_SIZE_LOC;
        fileSize = (inode[s] << 24) + (inode[s + 1] << 16) + (inode[s + 2] << 8) + inode[s + 3];
        printf("Size:\t\t%d bytes\n", fileSize);

        // Print times
        createdTime = (time_t *)&inode[FILE_CREATEDTIME_LOC];
        accessedTime = (time_t *)&inode[FILE_ACCESSTIME_LOC];
        modifiedTime = (time_t *)&inode[FILE_MODIFIEDTIME_LOC];
    } else if (inode[FILE_TYPE_FLAG_LOC] == FILE_TYPE_DIR) {
        printf("Directory\n");
        // Print times
        createdTime = (time_t *)&inode[DIR_CREATEDTIME_LOC];
        accessedTime = (time_t *)&inode[DIR_ACCESSTIME_LOC];
        modifiedTime = (time_t *)&inode[DIR_MODIFIEDTIME_LOC];
    }
    printf("Created:\t%s", ctime(createdTime));
    printf("Modified:\t%s", ctime(modifiedTime));
    printf("Accessed:\t%s", ctime(accessedTime));

    return TFS_SUCCESS;
}