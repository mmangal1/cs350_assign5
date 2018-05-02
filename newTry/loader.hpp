#ifndef LOADER_HPP
#define LOADER_HPP
#include <stdio.h>
#include "super_block.hpp"
#include <vector>

using namespace std;

class loader{
	public:
		void write_sb(string filename);
		void initialize(string filename);
		void load_inode_map(string filename);
		void load_fbl(string filename);
		void load_inodes(string filename);
		void load_sb(string filename);
	};
#endif
