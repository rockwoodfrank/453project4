#ifndef TINYFS_ERRNO_H
#define TINYFS_ERRNO_H

// SUCCESS MACRO
#define TFS_SUCCESS       			 0    

// GENERIC ERR MACROS
#define ERR_INVALID_INPUT       	-1    	// Invalid Input
#define SYS_ERR_MALLOC				-2		// system error for malloc
#define SYS_ERR_OPEN				-3		// system error for open
#define SYS_ERR_WRITE				-4		// system error for write
#define SYS_ERR_CLOSE				-5		// system error for close
#define SYS_ERR_SEEK				-6		// system error for seek
#define SYS_ERR_READ				-7		// system error for read

// LIBDISK ERR MACROS
#define ERR_DISK_FILE_NOT_FOUND		-10		// the given disk file does not exist and cannot be created
#define ERR_INVALID_DISK_FD			-11

// DISK ERR MACROS
#define ERR_NO_DISK_MOUNTED			-20		// calling tfs function with no disk mounted
#define ERR_DISK_OUT_OF_SPACE		-21		// out of free blocks on the disk
#define ERR_BAD_DISK				-22		// can't mount a improperly set up disk

// FILE ERR MACROS
#define ERR_INVALID_FD				-30		// calling tfs function for an invalid fd
#define ERR_OUT_OF_FDS				-31		// out of file descriptors
#define ERR_FILE_PNTR_OUT_OF_BOUNDS	-32		// trying to read beyond the bounds of the file

// DIR ERR MACROS
#define ERR_DIR_NOT_FOUND			-40		// directory does not exist
#define ERR_NOT_A_DIR				-41		// is a file, not a directory
#define ERR_DIR_ALREADY_EXISTS		-42		// trying to create a directory that already exists
#define ERR_DIR_NOT_EMPTY			-43		// trying to remove a directory that isn't empty

#endif