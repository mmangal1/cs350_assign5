#ifndef GLOBAL_HPP
#define GLOBAL_HPP
#include <stdio.h>
#include "super_block.hpp"
#include <vector>
#include "inode.hpp"

extern superblock sb;
extern int inode_map[256];
extern vector<inode*> inode_mem; //inodes on disk
extern int* free_block_list;
extern int fbl_block_count;
extern int read_free_mem_imap();
extern vector<int> read_fbl();
extern void update_inode_map(int index, int bit_to_set);
extern void update_free_list(int index, int bit_to_set);

#endif
