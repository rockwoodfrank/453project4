#ifndef LIB_TINY_HELPERS_H
#define LIB_TINY_HELPERS_H

#include "tinyFS_errno.h"
#include "libDisk.h"
#include "tinyFS.h"
#include "libTinyFS.h"

/* internal helper functions */
int     _update_fd_table_index();
char    _pop_free_block();
int     _free_block(char block_addr);
int     _parse_path(char* path, int index, char* buffer);
int     _navigate_to_dir(char* dirName, char* last_path_h, int* current_h, int* parent_h, int searching_for); 
int     _print_directory_contents(int block, int tabs);
int     _write_long(uint8_t* block, unsigned long longVal, char loc);
int     _remove_inode_and_blocks(char inode, char parent);
int     _fetch_parent(char inode_num);
int     _find_path_start(char *path);
int     _check_block_con(int diskNum, int block, int block_type, char* blocks_checked);

#endif