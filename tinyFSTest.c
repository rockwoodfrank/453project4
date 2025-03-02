#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#include "tinyFS.h"

void testTfs_mkfs();
void testTfs_mount();

int main(int argc, char *argv[]) {

    testTfs_mkfs();
    testTfs_mount();

    printf("> tinyFS Tests passed.\n");
    return 0;
}

void testTfs_mkfs()
{
    assert(1);
}

void testTfs_mount()
{
    
    assert(tfs_mount("testFiles/test.dsk") == 0);

    // tfs_unmount
    assert(tfs_unmount() == 0);
    // TODO: add some write function that verifies the disk can be written to

    // disks that don't exist
    // TODO: replace with actual error code
    assert(tfs_mount("testFiles/notAFile.dsk") == -1);
    //TODO: add some write function that verifies the 

    // remounting, making sure that doesn't break anything
    assert(tfs_mount("testFiles/test.dsk") == 0);
    assert(tfs_mount("testFiles/test2.dsk") == 0);
    // TODO: verify that test2 was written to and not test


    // files of the incorrect type
    // TODO: replace with actual error code
    assert(tfs_mount("testFiles/notADisk.txt") == -1);
    assert(tfs_mount("testFiles/notADisk.dsk") == -1);

}