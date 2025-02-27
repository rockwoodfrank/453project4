#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>

#include "readBlock.h"

int main(int argc, char *argv[])
{
    int testFile = open("disk1.dsk", O_RDWR);
    char *buff = (char *) malloc(sizeof(char) * BLOCKSIZE);
    readBlock(testFile, 1, (void *)buff);

    printf("%s\n", buff);

    // Checking for bad file number reads

    // Search reads that are too long or too short

    // Bad pointers

    // Bad lseek

    // Bad read

    
    printf("Tests passed.\n");
    return 0;
}