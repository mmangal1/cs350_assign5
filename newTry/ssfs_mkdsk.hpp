#ifndef SSFS_MKDSK_HPP
#define SSFS_MKDSK_HPP
#include <stdio.h>

class ssfs_mkdsk{
	public:
		ssfs_mkdsk(char* file_name, int num_blocks, int block_size);
		void write_fbl(int free_block_list[], char* file_name, int block_size, int num_blocks);
		void write_sb(int offset, char* file_name, int block_size, int num_blocks);
		void write_inode_map(int inode_map[], char* file_name, int block_size, int num_blocks);
	private:
		int fbl_block_count;
		int inode_map[256];
		int* free_block_list;
	};
#endif
