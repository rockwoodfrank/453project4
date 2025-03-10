#ifndef LIBTINY_H
#define LIBTINY_H

#include "libDisk.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

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
    #define FILE_DATA_LOC       (FILE_OFFSET_LOC + 4)                   // 22

    /* directory inode block byte locations */
    #define DIR_DATA_LOC        (FILE_NAME_LOC + FILENAME_LENGTH + 1)   // 14

    /* how many data blocks a file inode can hold */
    #define MAX_FILE_DATA       (BLOCKSIZE - FILE_DATA_LOC)             // 234

    /* how many inode blocks a directory inode can hold */
    #define MAX_DIR_INODES      (BLOCKSIZE - DIR_DATA_LOC)              // 242

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

/* Makes a blank TinyFS file system of size nBytes on the unix file
specified by ‘filename’. This function should use the emulated disk
library to open the specified unix file, and upon success, format the
file to be a mountable disk. This includes initializing all data to 0x00,
setting magic numbers, initializing and writing the superblock and
inodes, etc. Must return a specified success/error code. */
int tfs_mkfs(char *filename, int nBytes);

/* tfs_mount(char *diskname) “mounts” a TinyFS file system located within
‘diskname’. tfs_unmount(void) “unmounts” the currently mounted file
system. As part of the mount operation, tfs_mount should verify the file
system is the correct type. In tinyFS, only one file system may be
mounted at a time. Use tfs_unmount to cleanly unmount the currently
mounted file system. Must return a specified success/error code. */
int tfs_mount(char *diskname);
int tfs_unmount(void);

/* Creates or Opens a file for reading and writing on the currently
mounted file system. Creates a dynamic resource table entry for the file,
and returns a file descriptor (integer) that can be used to reference
this entry while the filesystem is mounted. */
fileDescriptor tfs_openFile(char *name);

/* Closes the file, de-allocates all system resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD);

/* Writes buffer ‘buffer’ of size ‘size’ BYTES, which represents an entire
file’s content, to the file system. Previous content (if any) will be
completely lost. Sets the file pointer to 0 (the start of file) when
done. Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD,char *buffer, int size);

/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD);

/* reads one byte from the file and copies it to buffer, using the
current file pointer location and incrementing it by one upon success.
If the file pointer is already past the end of the file then
tfs_readByte() should return an error and not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer);

/* change the file pointer location to offset (absolute). Returns
success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset);

#endif