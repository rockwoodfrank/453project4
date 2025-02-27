#include "libDisk.h"

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