#ifndef LOADER_HPP
#define LOADER_HPP
#include <stdio.h>
#include "super_block.hpp"
#include <vector>

using namespace std;

class loader{
	public:
		void write_sb(char *filename);
		void initialize(char *filename);
		void load_inode_map(char *filename);
		void load_fbl(char *filename);
		void load_inodes(char *filename);
		void load_sb(char *filename);
	};
#endif
