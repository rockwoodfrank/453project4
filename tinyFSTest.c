#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#include "tinyFS.h"

void testTfs_mkfs();
void testTfs_mount();
void testTfs_updateFile();
void* verify_contents(char *filePath, int location, size_t dataSize);

int main(int argc, char *argv[]) {
    
    testTfs_mkfs();
    testTfs_mount();

    printf("> tinyFS Tests passed.\n");
    return 0;
}

void testTfs_mkfs()
{  
    remove("testFiles/unitTestDisk.dsk");
    // Verifying the contents of the newly created disk are correct
    int diskSize = DEFAULT_DISK_SIZE;
    char *superBlock = (char *) malloc(sizeof(char) * (BLOCKSIZE+1));
    char *blankBlock = (char *) malloc(sizeof(char) * (BLOCKSIZE+1));
    FILE *newDisk;
    long int file_len;

    assert(tfs_mkfs("testFiles/unitTestDisk.dsk", diskSize) == 0);
    newDisk = fopen("testFiles/unitTestDisk.dsk", "r");

    fread(superBlock, sizeof(char), BLOCKSIZE, newDisk);
    // Test block type
    assert(superBlock[BLOCK_TYPE] == 0x1);
    // Test magic number
    assert(superBlock[SAFETY_BYTE] == 0x44);

    // Test free pointer
    assert(superBlock[FREE_PTR] == 0x01);

    // Empty
    assert(superBlock[3] == 0x0);

    // Test inode addresses are empty
    for (int i = FIRST_INODE_LOC; i < BLOCKSIZE; i++)
        assert(superBlock[i] == 0x0);

    // Test that the empty blocks are pointed to

    int counter = 0;
    uint8_t next_ptr = superBlock[FREE_PTR];
    while(next_ptr != 0x0)
    {
        counter++;
        fread(blankBlock, sizeof(char), BLOCKSIZE, newDisk);
        next_ptr = blankBlock[FREE_PTR];
    }
    assert(counter == (diskSize/BLOCKSIZE) -1);
    fclose(newDisk);

    // Test that running mkfs twice erases the disk: run twice with a smaller disk size
    diskSize = diskSize / 2;
    assert(tfs_mkfs("testFiles/unitTestDisk.dsk", diskSize) == 0);
    newDisk = fopen("testFiles/unitTestDisk.dsk", "r");

    fread(superBlock, sizeof(char), BLOCKSIZE, newDisk);
    // Test block type
    assert(superBlock[BLOCK_TYPE] == 0x1);
    // Test magic number
    assert(superBlock[SAFETY_BYTE] == 0x44);

    // Test free pointer
    assert(superBlock[FREE_PTR] == 0x01);

    // Empty
    assert(superBlock[3] == 0x0);

    // Test inode addresses are empty
    for (int i = FIRST_INODE_LOC; i < BLOCKSIZE; i++)
        assert(superBlock[i] == 0x0);

    // Test that the empty blocks are pointed to

    counter = 0;
    next_ptr = superBlock[FREE_PTR];
    while(next_ptr != 0x0)
    {
        counter++;
        fread(blankBlock, sizeof(char), BLOCKSIZE, newDisk);
        next_ptr = blankBlock[FREE_PTR];
    }
    assert(counter == (diskSize/BLOCKSIZE) -1);
    fclose(newDisk);

    remove("testFiles/unitTestDisk.dsk");


    // Testing with a weird blocksize
    assert(tfs_mkfs("testFiles/unitTestDisk.dsk", 927) == 0);
    newDisk = fopen("testFiles/unitTestDisk.dsk", "r");
    fseek(newDisk, 0L, SEEK_END);
    file_len = ftell(newDisk);
    
    assert(file_len == 768);
    fclose(newDisk);
    remove("testFiles/unitTestDisk.dsk");
    
    // Testing a disk size that's too big
    assert(tfs_mkfs("testFiles/unitTestDisk.dsk", BLOCKSIZE * (MAX_BLOCKS + 1)) != 0);
    assert(access("testFiles/unitTestDisk.dsk", F_OK) != 1);

    // Storing it in a directory that doesn't exist
    assert(tfs_mkfs("testFiles/doesntExist/unitTestDisk.dsk", DEFAULT_DISK_SIZE) != 0);
}

void testTfs_mount()
{
    int diskSize = DEFAULT_DISK_SIZE;
    assert(tfs_mkfs("testFiles/test.dsk", diskSize) == 0);
    assert(tfs_mount("testFiles/test.dsk") == 0);

    // tfs_unmount
    assert(tfs_unmount() == 0);
    // making sure that trying to unmount again results in failure
    assert(tfs_unmount() == -1);

    // disks that don't exist
    // TODO: replace with actual error code
    assert(tfs_mount("testFiles/notAFile.dsk") == -1);

    // remounting, making sure that doesn't break anything
    assert(tfs_mount("testFiles/test.dsk") == 0);
    assert(tfs_mount("testFiles/test2.dsk") == 0);
    // TODO: verify that test2 was written to and not test

    assert(tfs_unmount() == 0);
    // files of the incorrect type
    // TODO: replace with actual error code
    assert(tfs_mount("testFiles/notADisk.txt") == -1);
    assert(tfs_mount("testFiles/notADisk.dsk") == -1);
    

    // TODO: The specific case when a file isn't a disk: make sure a memory leak doesn't occur
}

void testTfs_updateFile()
{
    // Testing making a new file

    // Testing making a new file where the name is too long

    // Too many files

    // A file where the name is an empty string

    // Opening the same file twice

    // Writing some data to a file

    // Writing a weird size of data to a file

    // Writing to various files sequentially

    // Writing to a file that doesn't exist

    // Writing a file that's too big for the disk

    // Writing a bunch of files, deleting some, writing some more

    // Reading a byte from a file

    // Reading a lot of bytes from a file

    // Reading a byte out of range

    // Seeking to the beginning of a file

    // Seeking to the end of the file

    // Seeking to the middle of a file

    // Seeking in different files

    // Seeking out of range of the file

    // Deleting a file

    // Trying to delete the same file twice

    // Deleting all of the files on the disk

    // Deleting a file that doesn't exist

    // Writing to a deleted file

    //Closing the file
    
    // Trying to close a file twice

    // Closing a file that never existed


}

void* verify_contents(char *filePath, int location, size_t dataSize)
{
    FILE *readFile = fopen(filePath, "r");
    void *compareAgainst = malloc(dataSize);
    fseek(readFile, location, SEEK_SET);
    fread(compareAgainst, dataSize, 1, readFile);
    fclose(readFile);
    return compareAgainst;
}