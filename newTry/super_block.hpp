#ifndef SUPERBLOCK_HPP
#define SUPERBLOCK_HPP

	typedef struct superblock{
		int num_blocks;
		int block_size;
		int offset;
		int inode_offset;
	} superblock;
#endif
