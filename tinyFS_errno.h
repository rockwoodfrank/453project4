#ifndef TINYFS_ERRNO_H
#define TINYFS_ERRNO_H

#define TFS_SUCCESS       			 0    
#define ERR_INVALID_INPUT       	-1    	// Invalid Input
#define SYS_ERR_MALLOC				-2

#define ERR_NO_DISK_MOUNTED			-5
#define ERR_INVALID_FD				-5
#define ERR_OUT_OF_SPACE			-5
#define ERR_OUT_OF_FD				-5

// LIBDISK ERRS
#define ERR_DISK_NOT_FOUND			-10		// 
#define SYS_ERR_OPEN_DISK			-11
#define SYS_ERR_WRITE_DISK			-12
#define SYS_ERR_CLOSE_DISK			-13
#define SYS_ERR_SEEK_DISK			-14
#define SYS_ERR_READ_DISK			-15

// mounting / unmounting
#define ERR_BAD_DISK				-20
#define ERR_NOTHING_TO_UNMOUNT		-21

#define ERR_FILE_PNTR_OUT_OF_BOUNDS	-30

#define ERR_DIR_NOT_FOUND			-40
#define ERR_NOT_A_DIR				-41
#define ERR_DIR_ALREADY_EXISTS		-42
#define ERR_DIR_NOT_EMPTY			-43

#endif