#ifndef LIBTINY_H
#define LIBTINY_H

#include "libDisk.h"
#include <stdlib.h>
#include <stdio.h>

/* The default size of the disk and file system block */
#define BLOCKSIZE 256

/* 8 bit addressing for free blocks and inode data means max blocks is 256*/
#define MAX_BLOCKS 256

/* This macro represents the number of bytes left in the super block to store inode pointers */
#define MAX_INODES 251

#define FILE_NAMELENGTH 8

#define FD_TABLESIZE 256

/*Macros with universal data on every block*/
#define BLOCK_TYPE  0
#define SAFETY_BYTE 1
#define FREE_PTR    2

/*Macro for supernode location on disk*/
#define SUPERBLOCK_DISKLOC 0

/*Macros for inode locations*/
#define FILE_NAME   4
#define FILE_SIZE   13
#define DBLOCKS     14

/*The space on each file extent for data. Need to exclude one byte for type and safety byte*/
#define DATA_SPACE  254

/*Macros to store the block types*/
#define SUPERBLOCK  0x01
#define INODE       0x02
#define FILEEX      0x03
#define FREE        0x04



/* Your program should use a 10240 Byte disk size giving you 40 blocks
total. This is a default size. You must be able to support different
possible values */
#define DEFAULT_DISK_SIZE 10240

/* use this name for a default emulated disk file name */
#define DEFAULT_DISK_NAME "tinyFSDisk"

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
/*fd_table: the global variable used to store open file descriptors on memory.
The helper function _update_fd_table_index() sets the variable fd_table_index to
the next index so the program can store a new file*/
uint8_t fd_table[FD_TABLESIZE]; 

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

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire
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