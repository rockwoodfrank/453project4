#include "tinyFS.h"

tinyFS* mounted = NULL;
int fd_table_index = 0;

int _update_fd_table_index();


int tfs_mkfs(char *filename, int nBytes) {

    if(nBytes == 0) {
        return -1;
    }

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
    uint8_t buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE);
    int last_free_block;
    
    /* Initialize free blocks */
    buffer[0] = 0x04;
    buffer[1] = 0x44;
    buffer[2] = 0x01;
    for(int i=1; i< number_of_blocks; i++) {

        /* If you aren't at the last block, link to the next free block */
        if(i+1 != number_of_blocks) {
            buffer[2]++;
        } else {
            last_free_block = buffer[2];
            buffer[2] = 0x00;
        }

        if(writeBlock(disk_descriptor, i, buffer) < 0) {
            return -1;
        }
    }

    /* Initialize superblock */
    memset(buffer, 0, BLOCKSIZE);
    buffer[0] = 0x01;
    buffer[1] = 0x44;
    buffer[2] = 0x01;

    /* Store the tail of chain of free blocks on the superblock */
    buffer[4] = last_free_block;

    if(writeBlock(disk_descriptor, 0, buffer) < 0) {
        return -1;
    }

    return 0;

}

int tfs_mount(char* diskname) {
    int diskNum = openDisk(diskname, 0);
    /* Returns an error if that disk doesn't exist */
    if (diskNum < 0)
    {
        /* TODO: Diagnose and set an error number */
        return -1;
    }

    /* Returning an error if the file isn't formatted properly */
    /* Done by making sure byte 1 of the superblock is 0x44 */
    void *buffer = malloc(BLOCKSIZE);
    readBlock(diskNum, 0, buffer);
    uint8_t byte0 = ((uint8_t *)buffer)[0];
    uint8_t byte1 = ((uint8_t *)buffer)[1];
    if (byte0 != 1 || byte1 != 0x44)
    {
        free(buffer);
        /* TODO: Set the error number */
        return -1;
    }
    free(buffer);

    if (mounted != NULL)
        tfs_unmount();

    /* Initialize a new tinyFS object */
    mounted = (tinyFS *) malloc(sizeof(tinyFS));
    mounted->name = diskname;

    /* Opens the disk file */
    mounted->diskNum = diskNum;

    memset(fd_table, 0, FD_TABLESIZE);

    return 0;
}

int tfs_unmount() {
    /* TODO: Return the proper error number */
    if (mounted == NULL)
        return -1;
        
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

    /* Get the superblock */
    uint8_t superblock[256];
    readBlock(mounted->diskNum, 0, superblock);

    /* Buffer to hold inode data */
    char buffer[256];

    char* filename = (char*) malloc(9);
    
    /* Look through the inode pointers in the superblock */
    for(int i=0; i<MAX_INODES; i++) {
        if(!superblock[i+5]) {
            break;
        }
        readBlock(mounted->diskNum, superblock[i+5],buffer);
        filename = buffer + 4;
        if(strcmp(name,filename) == 0) {
            printf("Found file at inode %d\n", superblock[i+5]);
            if(_update_fd_table_index() >= 0) {
                fd_table[fd_table_index] = superblock[i+5];
                int temp = fd_table_index;
                fd_table_index++;
                return temp;
            } else {
                /* out of available file descriptors */
                return -1;
            }
        }
    }

    /* File not found, so create inode for it */
    uint8_t next_free_block = superblock[2];

    /* if there is an available free block... */
    if(next_free_block) {
        /* then, create inode for it*/
        readBlock(mounted->diskNum, next_free_block, buffer);

        /* update next free block */
        superblock[2] = buffer[2];
        
        /* update the inodes array */
        for(int i=0; i < MAX_INODES; i++) {
            if(!superblock[i+5]) {
                superblock[i+5] = next_free_block;
                break;
            }
        }

        /* put name of file on the inode */
        int z = 0;
        while(z < 8 && name[z] != '\000') {
            buffer[z+4] = name[z];
            z++;
        }
        buffer[0] = 0x02;
        buffer[2] = 0x00;


        if(_update_fd_table_index() >= 0) {
            fd_table[fd_table_index] = next_free_block;
            int temp = fd_table_index;
            fd_table_index++;

            /* turn the free block into an inode */
            if(writeBlock(mounted->diskNum, next_free_block, buffer) < 0) {
                return -1;
            }

            /* update the superblock */
            if(writeBlock(mounted->diskNum, 0, superblock) < 0) {
                return -1;
            }

            return temp;
        }

    /* Error if no available free block */
    } 

    return -1;
}

int tfs_closeFile(fileDescriptor FD) {

    /* if there is an entry, remove entry */
    if(fd_table[FD]) {
        fd_table[FD] = 0;
        return 0;
    }
    /* return error if there is no entry */
    return -1;
}

/* Helper function for to get the the next available fd table index */
int _update_fd_table_index() {
    int original = fd_table_index;

    /* Iterate to the end of the list */
    for(int i = fd_table_index; i < FD_TABLESIZE; i++) {
        if(fd_table[fd_table_index] == 0) {
            return fd_table_index;
        } else {
            fd_table_index++;
        }
    }

    /* Start over until your original starting point */
    fd_table_index = 0;
    for(int i=0; i<original; i++) {
        if(fd_table[fd_table_index] == 0) {
            return fd_table_index;
        } else {
            fd_table_index++;
        }
    }

    /* if you made it this far, there are no available file descriptors */
    return -1;
}

/* Uncomment and run this block if you want to test */
// int main() {
//     tfs_mkfs("SydneysDisk", 8192);
//     if(tfs_mount("SydneysDisk") < 0) {
//         printf("Failed to mount Sydney's disk");
//         return -1;
//     }

//     /* Existing files */
//     int fd = tfs_openFile("test1XXXIshouldnotseethis");
//     printf("File test1 has been opened with fd %d\n", fd);
//     printf("The inode of the file is %d\n\n", fd_table[fd]);

//     fd = tfs_openFile("test2");
//     printf("File test2 has been opened with fd %d\n", fd);
//     printf("The inode of the file is %d\n\n", fd_table[fd]);

//     fd = tfs_openFile("test3");
//     printf("File test3 has been opened with fd %d\n", fd);
//     printf("The inode of the file is %d\n\n", fd_table[fd]);

//     /* Make this file */
//     fd = tfs_openFile("test4");
//     printf("File test4 has been opened with fd %d\n", fd);
//     printf("The inode of the file is %d\n\n", fd_table[fd]);

//     if(tfs_closeFile(fd) < 0) {
//         printf("Closing file failed\n");
//         return -1;
//     }

//     fd = tfs_openFile("test5");
//     printf("File test5 has been opened with fd %d\n", fd);
//     printf("The inode of the file is %d\n\n", fd_table[fd]);

//     fd = tfs_openFile("test6");
//     printf("File test6 has been opened with fd %d\n", fd);
//     printf("The inode of the file is %d\n\n", fd_table[fd]);

//     return 0;
// }