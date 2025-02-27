#ifndef LIBDISK_H
#define LIBDISK_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include "tinyFS_errno.h"

#define BLOCKSIZE 256

int openDisk(char *filename, int nBytes);
int closeDisk(int disk);

/* readBlock() reads an entire block of BLOCKSIZE bytes from the open
disk (identified by 'disk') and copies the result into a local buffer
(must be at least of BLOCKSIZE bytes). The bNum is a logical block
number, which must be translated into a byte offset within the disk. The
translation from logical to physical block is straightforward: bNum=0
is the very first byte of the file. bNum=1 is BLOCKSIZE bytes into the
disk, bNum=n is n*BLOCKSIZE bytes into the disk. On success, it returns
0. -1 or smaller is returned if disk is not available (hasn't been 
opened) or any other failures. */
int readBlock(int disk, int bNum, void *block);

/* writes a <block> of bytes to the <disk> at the given block (bNum) */
int writeBlock(int disk, int bNum, void *block);

#endif