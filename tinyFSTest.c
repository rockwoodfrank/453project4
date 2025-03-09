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
    testTfs_updateFile();

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
    int diskSize = DEFAULT_DISK_SIZE;
    char diskName[25] = "testFiles/updateTest.dsk";
    remove(diskName);
    assert(tfs_mkfs(diskName, diskSize) == 0);
    assert(tfs_mount(diskName) == 0);

    // Testing making a new file
    fileDescriptor fileNum = tfs_openFile("test");
    assert(fileNum >= 0);
    char *inode = (char*) verify_contents(diskName, sizeof(char) * BLOCKSIZE * 1, sizeof(char) *BLOCKSIZE);
    assert(inode[BLOCK_TYPE_LOC] == INODE);
    assert(inode[SAFETY_BYTE_LOC] == 0x44);
    assert(inode[EMPTY_BYTE_LOC] == 0x00);
    assert(inode[FILE_TYPE_FLAG_LOC] == FILE_TYPE_FILE);
    //assert(strcmp(&(inode[FILE_NAME_LOC]), "test") == 0);
    int fileSize = ((int *)inode)[FILE_SIZE_LOC];
    assert(fileSize == 0);

    // Testing making a new file where the name is too long
    assert(tfs_openFile("thisnameistoolong.txt") < 0);

    // Too many files
    assert(tfs_unmount() == 0);
    assert(tfs_mkfs(diskName, diskSize) == 0);
    assert(tfs_mount(diskName) == 0);

    
    for (int i = 1; i < MAX_SUPBLOCK_INODES; i++)
    {
        char fileName[8];
        snprintf(fileName, 8, "fil%d", i);
        //assert(tfs_openFile(fileName) >= 0);
    }
    //assert(tfs_openFile("extra") < 0);
    assert(tfs_unmount() == 0);
    remove(diskName);

    // Trying to write when no disk is mounted
    //assert(tfs_openFile("nodrv") < 0);

    // A file where the name is an empty string
    assert(tfs_mkfs(diskName, DEFAULT_DISK_SIZE) == 0);
    assert(tfs_mount(diskName) == 0);

    //assert(tfs_openFile("") < 0);

    // Opening the same file twice
    int dupFileNum = tfs_openFile("abc");
    int dupFileNum2 = tfs_openFile("abc");
    assert(dupFileNum != dupFileNum2);

    assert(tfs_unmount() == 0);
    remove(diskName);

    // Writing some data to a file
    assert(tfs_mkfs(diskName, DEFAULT_DISK_SIZE) == 0);
    assert(tfs_mount(diskName) == 0);

    fileDescriptor wFileNum = tfs_openFile("test");
    assert(wFileNum >= 0);
    char testStr[44] = "The quick brown fox jumps over the lazy dog";
    assert(tfs_writeFile(wFileNum, testStr, 44) == 0);
    char *wFileinode = verify_contents(diskName, sizeof(char) * BLOCKSIZE * 1, sizeof(char) * BLOCKSIZE);
    char dataPtr = wFileinode[FILE_DATA_LOC];
    char *wFileData = verify_contents(diskName, sizeof(char) * BLOCKSIZE * dataPtr, sizeof(char) * BLOCKSIZE);
    
    assert(wFileData[BLOCK_TYPE_LOC] == 0x03);
    assert(wFileData[SAFETY_BYTE_LOC] == 0x44);
    assert(wFileData[EMPTY_BYTE_LOC] == 0x00);
    for (int i = FIRST_DATA_LOC; i < BLOCKSIZE; i++)
    {
        if (i < 44 + FIRST_DATA_LOC)
            assert(wFileData[i] == testStr[i - FIRST_DATA_LOC]);
        else
            assert(wFileData[i] == 0x00);
    }

    char newString[72] = "This is a brand new string. It should overwrite any data in the blocks.";
    assert(tfs_writeFile(wFileNum, newString, 72) == 0);
    wFileinode = verify_contents(diskName, sizeof(char) * BLOCKSIZE * 1, sizeof(char) * BLOCKSIZE);
    dataPtr = wFileinode[FILE_DATA_LOC];
    wFileData = verify_contents(diskName, sizeof(char) * BLOCKSIZE * dataPtr, sizeof(char) * BLOCKSIZE);
    
    assert(wFileData[BLOCK_TYPE_LOC] == 0x03);
    assert(wFileData[SAFETY_BYTE_LOC] == 0x44);
    assert(wFileData[EMPTY_BYTE_LOC] == 0x00);
    for (int i = FIRST_DATA_LOC; i < BLOCKSIZE; i++)
    {
        if (i < 72 + FIRST_DATA_LOC)
            assert(wFileData[i] == newString[i - FIRST_DATA_LOC]);
        else
            assert(wFileData[i] == 0x00);
    }

    char bigString[256] = "This is a very very looong string. It should take up 2 blocks. Lorem ipsum odor amet, consectetuer adipiscing elit. Libero curae hendrerit vel facilisis fames tellus quis nostra. Ac etiam risus in eu rutrum in.abcdefghijklmnopqrstuvwxyz1234567890qwertyuio";
    assert(tfs_writeFile(wFileNum, bigString, 256) == 0);
    wFileinode = verify_contents(diskName, sizeof(char) * BLOCKSIZE * 1, sizeof(char) * BLOCKSIZE);
    dataPtr = wFileinode[FILE_DATA_LOC];
    char dataPtr2 = wFileinode[FILE_DATA_LOC+1];
    wFileData = verify_contents(diskName, sizeof(char) * BLOCKSIZE * dataPtr, sizeof(char) * BLOCKSIZE);
    char *wFileData2 = verify_contents(diskName, sizeof(char) * BLOCKSIZE * dataPtr2, sizeof(char) * BLOCKSIZE);
    
    assert(wFileData[BLOCK_TYPE_LOC] == 0x03);
    assert(wFileData[SAFETY_BYTE_LOC] == 0x44);
    assert(wFileData[EMPTY_BYTE_LOC] == 0x00);
    for (int i = 0; i < 256; i++)
    {
        if (i < MAX_FILE_DATA)
            assert(wFileData[i + FIRST_DATA_LOC] == bigString[i]);
        // else
        //     assert(wFileData2[i + FIRST_DATA_LOC] == bigString[i]);
    }

    // Writing an integer to a file

    // Writing to various files sequentially

    // Writing to a file that doesn't exist

    // Writing a file that's too big for the disk

    // Writing a bunch of files, deleting some, writing some more

    assert(tfs_unmount() == 0);
    remove(diskName);

    // Reading a byte from a file
    assert(tfs_mkfs(diskName, DEFAULT_DISK_SIZE) == 0);
    assert(tfs_mount(diskName) == 0);
    fileDescriptor testFile = tfs_openFile("test");

    char readStr[6] = "hello";
    assert(tfs_writeFile(testFile, readStr, 6) == 0);
    char fileByte;
    for (int i = 0; i<6; i++)
    {
        assert(tfs_readByte(testFile, &fileByte) == 0);
        assert(fileByte == readStr[i]);
    }

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

    // Mounting and unmounting and making sure the data is still there

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