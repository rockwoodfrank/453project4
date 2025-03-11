#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#include "tinyFS.h"

void make_good_disk();
void main()
{
    make_good_disk();
    // Mounting a good system that's full of files
    assert(tfs_mount("testFiles/normalDisk.dsk") == TFS_SUCCESS);

    assert(tfs_readdir() == 0);

    tfs_unmount();

    // Tests for data blocks not on free list or allocated to an inode
    assert(tfs_mount("testFiles/weirdBlocks.dsk") != TFS_SUCCESS);
    // Tests for blocks that have been corrupted
    assert(tfs_mount("testFiles/lessCorrupted.dsk") != TFS_SUCCESS);

    // Tests where a block pointed to by an inode isn't a data block
    assert(tfs_mount("testFiles/weirdPointer.dsk") != TFS_SUCCESS);
}

void make_good_disk()
{
    tfs_mkfs("testFiles/normalDisk.dsk", DEFAULT_DISK_SIZE);
    tfs_mount("testFiles/normalDisk.dsk");

    fileDescriptor fileNums[20];
    fileNums[0] = tfs_openFile("/readme");
    tfs_writeFile(fileNums[0], "this is the readme for a normal disk file. This disk will have a lot of data for testing.", 90);
    tfs_createDir("/users");
    fileNums[5] = tfs_openFile("/users/hello");
    tfs_writeFile(fileNums[5], "a test, a test please work please.", 35);
    tfs_createDir("/users/rocky");
    tfs_createDir("/users/sydney");
    tfs_createDir("/users/quincy");
    fileNums[1] = tfs_openFile("/users/rocky/alist");
    tfs_writeFile(fileNums[1],  "Curse you Perry the Platypus.Curse you Perry the Platypus.Curse you Perry the Platypus.Curse you Perry the Platypus.Curse you Perry the Platypus.Curse you Perry the Platypus.Curse you Perry the Platypus.Curse you Perry the Platypus.Curse you Perry the Pla", 256);
    fileNums[2] = tfs_openFile("/users/quincy/rant");
    tfs_writeFile(fileNums[2], "Today when I walked into my economics class I saw something I dread every time I close my eyes. Someone had brought their new gaming laptop to class. The Forklift he used to bring it was still running idle at the back. I started sweating as I sat", 247);
    fileNums[3] = tfs_openFile("/users/sydney/speech");
    tfs_writeFile(fileNums[3], " Four score and seven years ago our fathers brought forth on this continent, a new nation, conceived in Liberty, and dedicated to the proposition that all men are created equal. Now we are engaged in a great civil war,", 219);
    tfs_createDir("/programs");
    fileNums[4] = tfs_openFile("/programs/hi.exe");
    tfs_writeFile(fileNums[4], "hello hello hello this is an executable file but its actually a text file", 74);

    tfs_unmount();
}