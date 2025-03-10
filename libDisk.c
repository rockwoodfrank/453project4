#include "libDisk.h"

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

    /* if nBytes is not evenly divisible by BLOCKSIZE */
    if(nBytes % BLOCKSIZE != 0) {
        /* change nBytes to the closet multiple of BLOCKSIZE that is less than nBytes */
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

int readBlock(int disk, int bNum, void *block) {
    int byteOffset = bNum * BLOCKSIZE;

	/* navigate to the correct block in the given disk */
    if (lseek(disk, byteOffset, SEEK_SET) == -1) {
        return -1;
    }
    
    /* reading from the file */
    if (read(disk, block, BLOCKSIZE) != BLOCKSIZE) {
        return -1;
    }

	/* return 0 on success */
    return 0;  
}

int writeBlock(int disk, int bNum, void* block) {
	int byteOffset = bNum * BLOCKSIZE;

	/* navigate to the correct block in the given disk */
	if (lseek(disk, byteOffset, SEEK_SET) == -1) {
		fprintf(stderr, "something went wrong with lseek\n");
		printf("errno num: %d, message: %s\n", errno, strerror(errno));
		return -1;
	}

	/* write the given block to the disk block */
	if (write(disk, block, BLOCKSIZE) == -1) {
		fprintf(stderr, "something went wrong with write\n");
		printf("errno num: %d, message: %s\n", errno, strerror(errno));
		return -1;
	}

	/* return 0 on success */
	return 0;
}