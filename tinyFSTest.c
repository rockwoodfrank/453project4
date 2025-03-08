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
    assert(superBlock[BLOCK_TYPE_LOC] == 0x1);
    // Test magic number
    assert(superBlock[SAFETY_BYTE_LOC] == 0x44);

    // Test free pointer
    assert(superBlock[FREE_PTR_LOC] == 0x01);

    // Empty
    assert(superBlock[3] == 0x0);

    // Test inode addresses are empty
    for (int i = FIRST_SUPBLOCK_INODE_LOC; i < BLOCKSIZE; i++)
        assert(superBlock[i] == 0x0);

    // Test that the empty blocks are pointed to

    int counter = 0;
    uint8_t next_ptr = superBlock[FREE_PTR_LOC];
    while(next_ptr != 0x0)
    {
        counter++;
        fread(blankBlock, sizeof(char), BLOCKSIZE, newDisk);
        next_ptr = blankBlock[FREE_PTR_LOC];
    }
    assert(counter == (diskSize/BLOCKSIZE) -1);
    fclose(newDisk);

    // Test that running mkfs twice erases the disk: run twice with a smaller disk size
    diskSize = diskSize / 2;
    assert(tfs_mkfs("testFiles/unitTestDisk.dsk", diskSize) == 0);
    newDisk = fopen("testFiles/unitTestDisk.dsk", "r");

    fread(superBlock, sizeof(char), BLOCKSIZE, newDisk);
    // Test block type
    assert(superBlock[BLOCK_TYPE_LOC] == 0x1);
    // Test magic number
    assert(superBlock[SAFETY_BYTE_LOC] == 0x44);

    // Test free pointer
    assert(superBlock[FREE_PTR_LOC] == 0x01);

    // Empty
    assert(superBlock[3] == 0x0);

    // Test inode addresses are empty
    for (int i = FIRST_SUPBLOCK_INODE_LOC; i < BLOCKSIZE; i++)
        assert(superBlock[i] == 0x0);

    // Test that the empty blocks are pointed to

    counter = 0;
    next_ptr = superBlock[FREE_PTR_LOC];
    while(next_ptr != 0x0)
    {
        counter++;
        fread(blankBlock, sizeof(char), BLOCKSIZE, newDisk);
        next_ptr = blankBlock[FREE_PTR_LOC];
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

}

void testTfs_mount()
{
    
    assert(tfs_mount("testFiles/test.dsk") == 0);
    // TODO: add some write function that verifies the disk can be written to

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

}