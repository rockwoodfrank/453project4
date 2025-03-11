#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#include "tinyFS.h"
#include "libTinyFS.h"

int main()
{
    char diskPath[24] = "testFiles/timeStampTest";
    tfs_mkfs(diskPath, DEFAULT_DISK_SIZE);
    tfs_mount(diskPath);
    int fileNum = tfs_openFile("test");
    tfs_writeFile(fileNum, "hello, world!", 14);
    tfs_closeFile(fileNum);

    sleep(2);
    fileNum = tfs_openFile("test");
    tfs_readFileInfo(fileNum);
    return 0;
}