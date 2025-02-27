#include <stdio.h>
#include <unistd.h>

#include "libDisk.h"

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