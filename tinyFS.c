#include "tinyFS.h"

tinyFS* mounted = NULL;

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

    /* Get the superblock */
    uint8_t superblock[256];
    readBlock(mounted->diskNum, 0, superblock);

    /* Buffer to hold inode data */
    char buffer[256];

    char* filename = (char*) malloc(9);
    
    /* Look through the inode pointers in the superblock */
    for(int i=0; i<MAX_INODES; i++) {
        printf("running loop\n");
        readBlock(mounted->diskNum, superblock[i+5],buffer);
        filename = buffer + 4;
        if(strcmp(name,filename) == 0) {
            printf("Found the inode for the file %s!! \n", filename);
            break;
        }
    }

    /* TODO: Now that files can be found, update tables */
    /* TODO: Implemetn if files are not found, they are created */

    return 0;
}

// int tfs_closeFile(fileDescriptor FD) {
    
// }


int main() {
    // tfs_mkfs("SydneysDisk", 8192);
    if(tfs_mount("SydneysDisk") < 0) {
        printf("Failed to mount Sydney's disk");
        return -1;
    }

    tfs_openFile("test5");
}