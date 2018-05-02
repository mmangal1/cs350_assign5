#ifndef INODE_HPP
#define INODE_HPP
#include <string>
using namespace std;

		class inode{
			public:
				void initialize(string file_name);
				inode(){};
				string file_name;
				int file_size;
				int total_blocks;
				int index;
				int direct_ptrs[12];
				int indirect_ptrs;
				int dindirect_ptrs;
			
		};
#endif
