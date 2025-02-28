#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#include "libDisk.h"
#include "tinyFS.h"

void libDiskTests();
void tinyFSTests();

int main(int argc, char *argv[]) {
    // Testing libDisk
    libDiskTests();

    tinyFSTests();

    printf("Tests passed.\n");
    return 0;
}

void libDiskTests()
{
    int testFile = open("test.dsk", O_RDWR);
    int compFile = open("test.dsk", O_RDWR);
    char *buff = (char *) malloc(sizeof(char) * BLOCKSIZE);
    char *buff2 = (char *) malloc(sizeof(char) * BLOCKSIZE);

    read(compFile, buff2, BLOCKSIZE);
    assert(readBlock(testFile, 0, (void *)buff) == 0);

    read(compFile, buff2, BLOCKSIZE);
    assert(readBlock(testFile, 1, (void *)buff) == 0);

    
    assert(strcmp(buff, buff2) == 0);

    // Checking for bad file number reads

    // Search reads that are too long or too short

    // Bad pointers

    // Bad lseek

    // Bad read
}

void tinyFSTests()
{
    char dummy = 'y';
    tfs_mount(&dummy);
}