#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#include "tinyFS.h"

void testTfs_mount();

int main(int argc, char *argv[]) {

    testTfs_mount();

    printf("tinyFS Tests passed.\n");
    return 0;
}

void testTfs_mount()
{
    char dummy = 'y';
    tfs_mount(&dummy);

    // tfs_mounting
}