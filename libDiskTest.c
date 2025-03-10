#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#include "libDisk.h"

void testReadBlock();

int main(int argc, char *argv[]) {
    // Testing libDisk
    testReadBlock();

    printf("> libDisk Tests passed.\n");
    return 0;
}

void testReadBlock()
{
    int testFile = open("testFiles/test.dsk", O_RDWR);
    int compFile = open("testFiles/test.dsk", O_RDWR);
    char *buff = (char *) malloc(sizeof(char) * BLOCKSIZE);
    char *buff2 = (char *) malloc(sizeof(char) * BLOCKSIZE);

    read(compFile, buff2, BLOCKSIZE);
    assert(readBlock(testFile, 0, (void *)buff) == -1);

    read(compFile, buff2, BLOCKSIZE);
    assert(readBlock(testFile, 1, (void *)buff) == -1);

    
    assert(strcmp(buff, buff2) == 0);

    // Checking for bad file number reads

    // Search reads that are too long or too short

    // Bad pointers

    // Bad lseek

    // Bad read
}