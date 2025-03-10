#include "libDisk.h"

int openDisk(char *filename, int nBytes) {
    /* check if the file exists */
    bool file_exists = access(filename, F_OK) == 0 ? true : false;

    /* if nBytes is between 0 and BLOCKSIZE OR nBytes is zero AND file does not exist */
    if (nBytes == 0 && !file_exists) {
        return ERR_DISK_FILE_NOT_FOUND;
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
        return SYS_ERR_OPEN;
    }

    /* if nBytes is not 0, write nByte 0's to the file */
    if(nBytes != 0) {
        uint8_t* buffer = (uint8_t*) malloc(nBytes);
        if(buffer == NULL) {
            return SYS_ERR_MALLOC;
        }
        memset(buffer, 0, nBytes);
        if(write(fd, buffer, nBytes) < 0) {
            return SYS_ERR_WRITE;
        }
        free(buffer);
    }  
    
    /* should always be > 3, since unix reserves fd 0, 1, & 2 */
    return fd;
}

int closeDisk(int disk) {
    /* make sure the given disk is valid */
    if (disk < 3) {
        return ERR_INVALID_DISK_FD;
    }

    /* close the disk */
    if(close(disk) < 0) {
        /* if errors with errno 9: bad file descriptor */
        if (errno == EBADF) {
            return ERR_INVALID_DISK_FD;
        }
        return SYS_ERR_CLOSE;
    }

    return TFS_SUCCESS;
}

int readBlock(int disk, int bNum, void *block) {
    int byteOffset = bNum * BLOCKSIZE;

	/* make sure the given disk is valid */
    if (disk < 3) {
        return ERR_INVALID_DISK_FD;
    }

	/* navigate to the correct block in the given disk */
	if (lseek(disk, byteOffset, SEEK_SET) == -1) {
        /* if errors with errno 9: bad file descriptor */
        if (errno == EBADF) {
            return ERR_INVALID_DISK_FD;
        }
		return SYS_ERR_SEEK;
	}
    
    /* reading from the file */
    if (read(disk, block, BLOCKSIZE) != BLOCKSIZE) {
        return SYS_ERR_READ;
    }

    return TFS_SUCCESS;  
}

int writeBlock(int disk, int bNum, void* block) {
    int byteOffset = bNum * BLOCKSIZE;

    /* make sure the given disk is valid */
    if (disk < 3) {
        return ERR_INVALID_DISK_FD;
    }

    /* make sure the given block is valid */
    if (block == NULL) {
        return ERR_INVALID_INPUT;
    }

	/* navigate to the correct block in the given disk */
	if (lseek(disk, byteOffset, SEEK_SET) == -1) {
        /* if errors with errno 9: bad file descriptor */
        if (errno == EBADF) {
            return ERR_INVALID_DISK_FD;
        }
		return SYS_ERR_SEEK;
	}

	/* write the given block to the disk block */
	if (write(disk, block, BLOCKSIZE) == -1) {
		return SYS_ERR_WRITE;
	}

	return TFS_SUCCESS;
}