#include "libTinyFS_helpers.h"

int ERR;
tinyFS* mounted;
uint8_t fd_table[FD_TABLESIZE];
int fd_table_index; 

/* ~ HELPER FUNCTIONS ~ */

/* _check_block_con(): checks that the given block is of the given block_type 
    + fills out the blocks_checked flag array for each block it checks
    - errors if the block is not formatted as the type specified */
/* if given the superblock, it will check each block in the tfs (if the tfs is correct) */
int _check_block_con(int diskNum, int block, int block_type, char* blocks_checked) {
    blocks_checked[block] = 1;

    char buffer[BLOCKSIZE];
    if ((ERR = readBlock(diskNum, block, buffer)) < 0) {
        return ERR;
    }

    char byte0 = buffer[BLOCK_TYPE_LOC];
    char byte1 = buffer[SAFETY_BYTE_LOC];
    char byte2 = buffer[FREE_PTR_LOC];
    char byte3 = buffer[EMPTY_BYTE_LOC];
    blocks_checked[block] = 1;
    if (block_type == SUPERBLOCK) {
        /* check the first four bytes */
        if (byte0 != SUPERBLOCK || byte1 != SAFETY_HEX || byte3 != EMPTY_TABLEVAL) {
            printf("(30)\n");
            return ERR_BAD_DISK;
        }

        if (byte2 != 0) {
            if ((ERR = _check_block_con(diskNum, byte2, FREE, blocks_checked)) < 0) {
                return ERR;
            }
        }

        // check that everything in the inode is a data block / inode block (for dirs)
        for (int i = FIRST_SUPBLOCK_INODE_LOC; i < MAX_SUPBLOCK_INODES + FIRST_SUPBLOCK_INODE_LOC; i++) {
            if ((buffer[i] != 0) && ((ERR = _check_block_con(diskNum, buffer[i], INODE, blocks_checked)) < 0)) {
                return ERR;
            }
        }
    }
    else if (block_type == INODE) {
        /* check the first four bytes */
        if (byte0 != INODE || byte1 != SAFETY_HEX || byte2 != EMPTY_TABLEVAL || byte3 != EMPTY_TABLEVAL) {
            printf("(50)\n");
            return ERR_BAD_DISK;
        }
        /* check that the file type flag is valid */
        int file_type = buffer[FILE_TYPE_FLAG_LOC];
        if (file_type != FILE_TYPE_DIR && file_type != FILE_TYPE_FILE) {
            printf("(56)\n");
            return ERR_BAD_DISK;
        }
     
        /* check that the name is valid */
        char* filename = buffer + FILE_NAME_LOC;
        if (buffer[FILE_NAME_LOC + FILENAME_LENGTH] != 0 || strlen(filename) <= 0) {
            printf("(63)\n");
            return ERR_BAD_DISK;
        }

        /* check that everything in the inode is a data block / inode block (for dirs) */
        int num_data = 0;
        int size = 0;
        int start_bound = file_type == FILE_TYPE_FILE ? FILE_DATA_LOC : DIR_DATA_LOC;
        int range = file_type == FILE_TYPE_FILE ? MAX_FILE_DATA : MAX_DIR_INODES;
        for (int i = start_bound; i < range; i++) {
            if (buffer[i] != 0) {
                /* make sure file inodes only have data blocks, and count how many*/
                if (file_type == FILE_TYPE_FILE) {
                    num_data++;
                    if ((ERR = _check_block_con(diskNum, buffer[i], FILEEX, blocks_checked)) < 0) {
                        return ERR;
                    }

                    /* grab the size, to check if the number of data blocks correlates */
                    int s = FILE_SIZE_LOC;
                    size = (buffer[s] << 24) + (buffer[s + 1] << 16) + (buffer[s + 2] << 8) + buffer[s + 3];

                /* make sure directory inodes only contain inode blocks*/
                } else if (file_type == FILE_TYPE_DIR  && (_check_block_con(diskNum, buffer[i], INODE, blocks_checked) != 0)) {
                    return ERR_BAD_DISK;
                }
            }
        }

        /* if a file, make sure the amount of data blocks correlates to the amount of data blocks it has*/
        if (file_type == FILE_TYPE_FILE && (((size-1) / MAX_DATA_SPACE + 1 != num_data))) {
            if(!(size == 0 && num_data == 0)) {
                return ERR_BAD_DISK;
            }
        }
    }

    else if (block_type == FILEEX) {
        /* check the first four bytes */
        if (byte0 != FILEEX || byte1 != SAFETY_HEX || byte2 != EMPTY_TABLEVAL || byte3 != EMPTY_TABLEVAL) {
            return ERR_BAD_DISK;
        }
    }

    else if (block_type == FREE) {
        /* check the first four bytes */
        if (byte0 != FREE || byte1 != SAFETY_HEX || byte3 != EMPTY_TABLEVAL) {
            return ERR_BAD_DISK;
        }

        if (byte2 != 0) {
            return _check_block_con(diskNum, byte2, FREE, blocks_checked);
        }
    }
    
    return TFS_SUCCESS;
}

/* _navigate_to_dir(): 'navigates' to the last dir (or file) if the given dirName path
    + fills the last_path_h with the name of the dir/file at the end of the dirName path
    + fills the current_h (if given) with the inode block address of the last inode it finds in the dirName path
    + fills the parrent_h (if given) with the inode block address of the parent of current_h
    = searching_for specifies if the last thing in the path should be a file or a directory
    > returns wether or not it found the dir/file at the end of the dirName path
    - errors:
        - if any item along the path is incorrectly a file rather than directory
        - if a directory along that path cannot be found */
int _navigate_to_dir(char* dirName, char* last_path_h, int* current_h, int* parent_h, int searching_for) {

    /* The superblock effectively behaves as the inode for the root */
    int current = SUPERBLOCK_DISKLOC;
    char current_block[BLOCKSIZE]; 
    int parent = current;
    if ((ERR = readBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, current_block)) < 0) {
        return ERR;
    }

    bool dir_found_flag = false; 
    int path_index = _find_path_start(dirName);
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
        int range = current == 0 ? MAX_SUPBLOCK_INODES : MAX_DIR_INODES;
        for(int i = start_bound; i < range + start_bound; i++) {

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

/* Pop and return the next free block, and replace the parent index
 with that block's next block. Should return 0 if no more free blocks exist. */
char _pop_free_block() {
    // Grab the superblock. This is done locally as some functions may not
    // need to store the superblock so this function does it just in case
    char superblock[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock)) < 0) {
        return ERR;
    }

    char newBlock[BLOCKSIZE];
    char next_free_block = superblock[FREE_PTR_LOC];
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
int _free_block(char block_addr) {
    // Grab the superblock
    char superblock[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, SUPERBLOCK_DISKLOC, superblock)) < 0) {
        return ERR;
    }

    // Change block type
    char clean_block[BLOCKSIZE];
    clean_block[BLOCK_TYPE_LOC] = FREE;
    clean_block[SAFETY_BYTE_LOC] = SAFETY_HEX;
    clean_block[EMPTY_BYTE_LOC] = EMPTY_TABLEVAL;
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

    char directory_inode[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, block, directory_inode)) < 0) {
        return ERR;
    }

    char inode[BLOCKSIZE];
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

/* given a file inode number and its parent, delete it */
int _remove_inode_and_blocks(char inode_num, char parent) {
    /* Grab the block's inode */
    char inode[BLOCKSIZE]; 
    if ((ERR = readBlock(mounted->diskNum, inode_num, inode)) < 0 ) {
        return ERR;
    }

    // Resetting the direct blocks so the data is "lost" since nothing is pointing to them
    // Counter for clearing
    int i = 0;
    char dir_block = inode[FILE_DATA_LOC + i++];
    while(dir_block != 0x0) {
        // Allocating each block as "free" and adding them to the linked list
        if ((ERR = _free_block(dir_block)) < 0) {
            return ERR;
        }
        dir_block = inode[FILE_DATA_LOC + i++];
    }

    /* Finally, freeing the inode */
    if ((ERR = _free_block(inode_num)) < 0) {
        return ERR;
    }

    /* close the file in the fd table and if there are 
        multiple FDs for the file, close those too */
    for(int i = 0; i < FD_TABLESIZE; i++) {
        if(fd_table[i] == inode_num) {
            if ((ERR = tfs_closeFile(i)) < 0) {
                return ERR;
            }
        }
    }
    // Remove the inode number from the parent_block
    char parent_block[BLOCKSIZE];
    if ((ERR = readBlock(mounted->diskNum, parent, parent_block)) < 0 ) {
        return ERR;
    }

    i = parent == SUPERBLOCK_DISKLOC ? FIRST_SUPBLOCK_INODE_LOC : DIR_DATA_LOC;
    while (parent_block[i] != inode_num) i++;

    parent_block[i] = EMPTY_TABLEVAL;
    if ((ERR = writeBlock(mounted->diskNum, parent, parent_block)) < 0 ) {
        return ERR;
    }

    return TFS_SUCCESS;
}

/* given an inode, finds the parent */
int _fetch_parent(char inode_num) {

    struct stat file_stat;
    if (fstat(mounted->diskNum, &file_stat) == -1) {
        return SYS_ERR_FSTAT;
    }  

    int num_blocks = file_stat.st_size / BLOCKSIZE; 

    char buffer[BLOCKSIZE];
    for(int i=0; i < num_blocks; i++) {

        if((ERR = readBlock(mounted->diskNum, i, buffer)) < 0) {
            return ERR;
        }

        if(i == SUPERBLOCK_DISKLOC) {
            for(int j = FIRST_SUPBLOCK_INODE_LOC; j < FIRST_SUPBLOCK_INODE_LOC + MAX_SUPBLOCK_INODES; j++) {
                if(buffer[j] == inode_num) {
                    return SUPERBLOCK_DISKLOC;
                }
            }
        } else {
            if(buffer[BLOCK_TYPE_LOC] == INODE && buffer[FILE_TYPE_FLAG_LOC] == FILE_TYPE_DIR) {
                for(int j = DIR_DATA_LOC; j < DIR_DATA_LOC + MAX_DIR_INODES; j++) {
                    if(buffer[j] == inode_num) {
                        return i;
                    }
                }
            }
        }
    }

    return ERR_BAD_DISK;
}

// Writing longs to a block, specifically for timestamps
int _write_long(uint8_t* block, unsigned long longVal, char loc) { 
    char *longConverted = (char *)&longVal;
    for (int i = 0; i < 8; i ++) {
        block[loc+i] = longConverted[i];
    }
    return 0;
}

// Formatting the path name
int _find_path_start(char *path)
{
    if (path[0] != '/')
    {
        return 0;
    }
    return 1;
}