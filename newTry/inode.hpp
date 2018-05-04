#ifndef INODE_HPP
#define INODE_HPP
#include <string.h>

typedef struct inode{
	inode() {
		memset(file_name, 0, sizeof file_name); 
		memset(direct_ptrs, 0, sizeof direct_ptrs); 
	}
	char file_name[32];
	int file_size;
	int total_blocks;
	int index;
	int direct_ptrs[12];
	int indirect_ptrs;
	int dindirect_ptrs;
} inode;
#endif
