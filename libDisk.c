#include "libDisk.h"

int openDisk(char *filename, int nBytes) {
    /* check if the file exists */
    bool file_exists = access(filename, F_OK) == 0 ? true : false;

    /* if nBytes is between 0 and BLOCKSIZE OR nBytes is zero AND file does not exist */
    if (nBytes == 0 && !file_exists) {
        return ERR_DISK_NOT_FOUND;
    }
    if((nBytes < BLOCKSIZE && nBytes != 0)) {
        return ERR_INVALID_INPUT;
    }

    /* make sure nBytes is evenly divisible by BLOCKSIZE */
    if(nBytes % BLOCKSIZE != 0) {
        nBytes = (nBytes / BLOCKSIZE) * BLOCKSIZE;
    }

    int fd;

    /* if the file does not exist, create the file */
    if (!file_exists) {
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
        return SYS_ERR_OPEN_DISK;
    }

    /* if nBytes is not 0, write nByte 0's to the file */
    if(nBytes != 0) {
        uint8_t* buffer = (uint8_t*) malloc(nBytes);
        if(buffer == NULL) {
            return SYS_ERR_MALLOC;
        }
        memset(buffer, 0, nBytes);
        if(write(fd, buffer, nBytes) < 0) {
            return SYS_ERR_WRITE_DISK;
        }
        free(buffer);
    }  
    
    return fd;
}

int closeDisk(int disk) {
    /* close the disk */
    if(close(disk) < 0) {
        return SYS_ERR_CLOSE_DISK;
    }

    return TFS_SUCCESS;
}

int readBlock(int disk, int bNum, void *block) {
    int byteOffset = bNum * BLOCKSIZE;

	/* navigate to the correct block in the given disk */
    if (lseek(disk, byteOffset, SEEK_SET) == -1) {
        return SYS_ERR_SEEK_DISK;
    }
    
    /* reading from the file */
    if (read(disk, block, BLOCKSIZE) != BLOCKSIZE) {
        return SYS_ERR_READ_DISK;
    }

    return TFS_SUCCESS;  
}

int writeBlock(int disk, int bNum, void* block) {
	int byteOffset = bNum * BLOCKSIZE;

	/* navigate to the correct block in the given disk */
	if (lseek(disk, byteOffset, SEEK_SET) == -1) {
		fprintf(stderr, "something went wrong with lseek\n");
		printf("errno num: %d, message: %s\n", errno, strerror(errno));
		return SYS_ERR_SEEK_DISK;
	}

	/* write the given block to the disk block */
	if (write(disk, block, BLOCKSIZE) == -1) {
		fprintf(stderr, "something went wrong with write\n");
		printf("errno num: %d, message: %s\n", errno, strerror(errno));
		return SYS_ERR_WRITE_DISK;
	}

	return TFS_SUCCESS;
}