FEATURES: BCEH

Rockwood Frank, Quincy Collinsworth, Sydney Lynch

Project 4 Fully Functional

Notes if Issues with auto-grader:
- tinyFS_errno.h 
	the basicTinyFSTest.c given to use in class uses "TinyFS_errno.h" but the specs use "tinyFS_errno.h"; we are using tinyFS_errno.h (lower case t)
- tfs_openFile() & direcotry functions (discussed with Tomas)
	the instructions say to use the absolute path for files and directories, which we took to mean that paths should include the "/" before it. But, the given tests do not use "/" so we have implemented it to accomodate either. For example, tfs_openFile("test") and tfs_openFile("/test") should both work.
- all of our MACROS are in tinyFS.h and libTinyFS.h holds the tfs library function headers

Explanation of our TFS:
- The FREE_PTR_BYTE in our superblock and free blocks are used to point to the next available free block. These work in a linked list manner and determine which block to use for write new files and inodes. If the free block pointer is 0, there are no more free blocks.
- Each byte in our superblock points to an inode.
- Each inode has file information, at locations specified by MACROS, and the remaining bytes point to file data blocks (or point to inode blocks if a directory inode instead of a file inode).
- We have a gloabl list of file dsecriptors that map to its corresponding file inode.


Chosen Features:

Feature (B): Directory listing and file renaming (10%)
- tfs_rename(fileDescriptor FD, char* newName)
	We implement this by finding given inode file and then replacing the filename with the given new name. We checked that this worked by renaming a file then trying to open the new name file and read from it to make sure its content remains constant, and we check its existence in the output of tfs_readdir(). The assignment specifications do not specify what naming restrictions are to be implemented, so we decided to make it an intentional feature that users could create duplicate names through the rename function. 
- tfs_readdir()
	We implimemnted this by printing every directory and file in our tfs, with tab indentation to show the nested directories. To show that this works, we added various files and directories to our demo disk and then call the tfs_readdir() function to display the contents, and make sure that it shows all the expected files/directories.


Feature (C): Hierarchical directories (20%)
- For our directories, we decided to have the superblock function as the root directory inode. To integrate directories, we changed our inode block to hold a FILE_TYPE_FLAG that can be either f for file or d for directory. The file inodes have more information stored for the file, whereas directory inodes only have the name and the inodes that belong to that directory. We have various MACROS to support this implementation. 
- Since the working directory is always the root directory, the absolute path always needs to be specified. To account for this, we made a helper _navigate_to_dir() that traverses the given absolute path name and returns needed information.
- To ensure that our directories were working, we spent a lot of time looking at the disk output. We also have various tests (that can be run with "make test") that check edge cases. The basic functionality (nested directories and minimal edge cases) is represented in our demo.


Feature (E): Timestamps (10%)
- The created, modified, and accessed timestamps are stored in the metadata of each inode, and uses the Unix time standard to store and process each timestamp. tfs_openFile() determines the create time, tfs_writeFile() and tfs_rename() determines the modified time, and tfs_readByte() determines the accessed time by getting the current time. 
- To test this, one of our test functions sleeps for a short amount of time to test time differences. The tfs_readFileInfo will get a file's name, size, and these 3 timestamps to display to the output. While these values currently cannot be directly accessed outside of readFileInfo(), they are stored in the UNIX time standard, and so allow for high levels of backwards compatibility.


Feature (H): Implement file system consistency checks (10%)
- To check the file system consistency, we make sure that the given disk file is fully correct before mounting. We do this with a helper fuction: _check_block_con(), which checks that the given block matches the given block type. 
- The helper checks that the first four bytes match what is expected for the given block type, and then depending on the block type it checks the rest of the block content. 
- We check that every block referenced in the system has valid information, but for inodes we also ensure that the filesize is accurate (to a margin of one block) to minimize risk of accessing data out of its bounds or segfaulting if someone where to corrupt a file.
- By passing in the superblock, this helper recursivley checks each block it can reach from the superblock, which is all files and directories it holds (which then recursively checks the files and directories) as well as checking the free blocks through the free block pointer stored in the superblock. Each block that it checks gets marked that it was checked in a separate array. After the function returns, if any blocks were not considered checked by the helper function, those blocks are unreachable from the superblock, and therefore the disk is corrupted. 


Here is the outline to our storage structures:

Superblock:
Byte    Value
0       1
1       0x44
2       Ptr to next free block
3       Empty
4+      inode addresses

Inode (if file):
Byte    Value
0       2
1       0x44
2       Empty
3       Empty
4		File Type Flag (f)
5-13    File name
14-17   File size
18-21   File Offset
22-29 	File Creation Time
30-37	File Modification Time
38-45	File Access Time
46+ 	Direct Blocks to Data

Inode (if directory):
Byte    Value
0       2
1       0x44
2       Empty
3       Empty
4		File Type Flag (d)
5-13    Dir name
14-21 	Dir Creation Time
22-29	Dir Modification Time
30-37	Dir Access Time
38+ 	Direct Blocks to Inodes

File Extent:
Byte    Value
0       3
1       0x44
2       Empty
3       Empty
4+      Data

Free Block:
Byte    Value
0       4
1       0x44
2       Next Free Block
3+      Zeros or junk data
