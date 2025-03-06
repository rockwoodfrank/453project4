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
    // Verifying the contents of the newly created disk are correct

    // Testing with a weird blocksize

    // Storing it in a directory that doesn't exist

    // Disk files that already exist
    assert(1);
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