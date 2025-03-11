#ifndef LIBTINY_H
#define LIBTINY_H

#include "libDisk.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

/* use this name for a default emulated disk file name */
#define DEFAULT_DISK_NAME "tinyFSDisk"

/* ~ MACROS FOR DEFAULT SIZES ~ */

    /* Your program should use a 10240 Byte disk size giving you 40 blocks
    total. This is a default size. You must be able to support different
    possible values */
    #define DEFAULT_DISK_SIZE 10240

    /* The default size of the disk and file system block */
    #define BLOCKSIZE 256

    /* 8 bit addressing for free blocks and inode data means max blocks is 256 */
    #define MAX_BLOCKS 256

    /* the amount of FDs able to be open at once for the file system */
    #define FD_TABLESIZE 256

/* ^ MACROS FOR DEFAULT SIZES ^ */    

/* standardized block information byte locations */
#define NUM_RESERVED_BYTES  4
#define BLOCK_TYPE_LOC      0
#define SAFETY_BYTE_LOC     1
#define FREE_PTR_LOC        2
#define EMPTY_BYTE_LOC      3

/* block types */
#define SUPERBLOCK  0x01
#define INODE       0x02
#define FILEEX      0x03
#define FREE        0x04

/* the value of the safety byte for each block */
#define SAFETY_HEX  0x44

/* standard empty table value */
#define EMPTY_TABLEVAL 0

/* ~ MACROS FOR SUPER BLOCK ~ */
    /* where the superblock is located in the disk */
    #define SUPERBLOCK_DISKLOC          0

    /* where the first inode is stored and how many inodes it can hold */
    #define FIRST_SUPBLOCK_INODE_LOC    (0 + NUM_RESERVED_BYTES)                // 4
    #define MAX_SUPBLOCK_INODES         (BLOCKSIZE - FIRST_SUPBLOCK_INODE_LOC)  // 252

/* ^ MACROS FOR SUPER BLOCK ^ */

/* ~ MACROS FOR INODE BLOCK ~ */
    /* inode block constants */
    #define FILE_TYPE_FILE      0x66                                    // f
    #define FILE_TYPE_DIR       0x64                                    // d
    #define FILENAME_LENGTH     8

    /* inode block byte locations */
    #define FILE_TYPE_FLAG_LOC  (0 + NUM_RESERVED_BYTES)                // 4
    #define FILE_NAME_LOC       (FILE_TYPE_FLAG_LOC + 1)                // 5

    /* file inode block byte locations */
    #define FILE_SIZE_LOC       (FILE_NAME_LOC + FILENAME_LENGTH + 1)   // 14
    #define FILE_OFFSET_LOC     (FILE_SIZE_LOC + 4)                     // 18
        
        /* timestamp macros */
        #define FILE_CREATEDTIME_LOC    (FILE_OFFSET_LOC + 4)
        #define FILE_MODIFIEDTIME_LOC   (FILE_CREATEDTIME_LOC + 8)
        #define FILE_ACCESSTIME_LOC     (FILE_MODIFIEDTIME_LOC + 8)

    #define FILE_DATA_LOC       (FILE_ACCESSTIME_LOC + 8)              
  
    /* directory inode block byte locations */

        /* directory timestamp macros, since you guys decided to make them different :/ */
        #define DIR_CREATEDTIME_LOC    (FILE_OFFSET_LOC + 4)
        #define DIR_MODIFIEDTIME_LOC   (DIR_CREATEDTIME_LOC + 8)
        #define DIR_ACCESSTIME_LOC     (DIR_MODIFIEDTIME_LOC + 8)

    #define DIR_DATA_LOC        (DIR_ACCESSTIME_LOC + 8)   

    /* how many data blocks a file inode can hold */
    #define MAX_FILE_DATA       (BLOCKSIZE - FILE_DATA_LOC)

    /* how many inode blocks a directory inode can hold */
    #define MAX_DIR_INODES      (BLOCKSIZE - DIR_DATA_LOC) 

/* ^ MACROS FOR INODE BLOCK ^ */

/* ~ MACROS FOR DATA/FILE-EXTENT/FREE BLOCKS */
    /* starting location of data in the data block */
    #define FIRST_DATA_LOC      (0 + NUM_RESERVED_BYTES)                // 4

    /* amount of bytes on a data block reserved for data */
    #define MAX_DATA_SPACE      (BLOCKSIZE - FIRST_DATA_LOC)            // 252

/* ^ MACROS FOR DATA/FILE-EXTENT BLOCKS ^ */

typedef struct tinyFS {
    // Name of the disk file
    char *name;
    // Disk number returned by openDisk()
    int diskNum;
} tinyFS;

/* use as a special type to keep track of files. This value serves as the
index into the file descriptor table */
typedef int fileDescriptor;
extern int fd_table_index;

#endif