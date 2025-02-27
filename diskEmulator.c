#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define BLOCKSIZE 256

int openDisk(char *filename, int nBytes) {

    bool file_exists = false;

    /* if the file exists... */
    if(access(filename, F_OK) == 0) {
        /* set file_exists to true*/
        file_exists = true;
    }

    /* if nBytes is between 0 and BLOCKSIZE OR nBytes is zero AND file does not exist */
    if((nBytes < BLOCKSIZE && nBytes != 0) || (nBytes == 0 && !file_exists)) {
        /* Error */
        return -1;
    }

    /* if nBytes is not evenly divisible by BLOCKSIZE*/
    if(nBytes % BLOCKSIZE != 0) {
        /* change nBytes to the closet multiple of BLOCKSIZE that is less than nBytes*/
        nBytes = (nBytes / BLOCKSIZE) * BLOCKSIZE;
    }

    int fd;

    /* if the file does not exist... */
    if (!file_exists) {
        /* create the file */
        fd = open(filename, O_RDWR | O_CREAT, 0644);

    /* if nBytes is zero and file exists, open existing  file */
    } else if(nBytes == 0) {
        fd = open(filename, O_RDWR, 0644);

    /* if nBytes is not zero and file exists, open the file and overwrite its contents */
    } else {
        fd = open(filename, O_RDWR | O_TRUNC, 0644);
    }

    /* error checking open() system call */
    if(fd < 0) {
        return -1;
    }

    /* if nBytes is not 0...*/
    if(nBytes != 0) {
        /* write nByte 0's to the file */
        uint8_t* buffer = (uint8_t*) malloc(nBytes);
        if(buffer == NULL) {
            return -1;
        }
        memset(buffer, 0, nBytes);
        if(write(fd, buffer, nBytes) < 0) {
            return -1;
        }
        free(buffer);
    }  
    
    return fd;
}

int closeDisk(int disk) {

    /* close the disk */
    if(close(disk) < 0) {
        return -1;
    }

    return 0;
}

int main() {
    /* file exists with nBytes greater than BLOCKSIZE that is divisible by BLOCKSIZE and the file should be truncated */
    openDisk("test1.txt", 32);

    /* file exists with nBytes greater than BLOCKSIZE that is not divisible by BLOCKSIZE and the file should be truncated */
    openDisk("test2.txt", 34);

    /* file exists with nBytes less than BLOCKSIZE. This should return -1 */
    int output = openDisk("test3.txt", 10);
    if(output == -1) {
        printf("Error checking works in case that the file exists with input less than BLOCKSIZE\n");
    }

    /* file exists with nBytes being 0 and the file should be untouched */
    openDisk("test4.txt", 0);

    /* file does not exist and nBytes is greater than BLOCKSIZE and divisible by BLOCKSIZE */
    openDisk("test5.txt", 32);

    /* file does not exist and nBytes is greater than BLOCKSIZE and NOT divisible by BLOCKSIZE */
    openDisk("test6.txt", 34);

    /* file does not exist and nByte is less than BLOCKSIZE*/
    output = openDisk("test7.txt", 12);
    if(output == -1) {
        printf("Error checking works in case that the file does not exist with input less than BLOCKSIZE\n");
    }

    output = openDisk("test8.txt", 12);
    if(output == -1) {
        printf("Error checking works in case that the file does not exist with input less than BLOCKSIZE that is zero\n");
    }





}
