#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include <fstream>
#include <bitset>
#include "loader.hpp"
#include <sys/stat.h>
#include "global.hpp"
#include "inode.hpp"
#include "super_block.hpp"


using namespace std;


/* Loads superblock into sb struct */
void loader::load_sb(string filename){
	FILE *fp = fopen(filename.c_str(), "rb");
	fseek(fp, 0, SEEK_SET);
	int num = 0;
	fread(&(sb.num_blocks), sizeof(int), 1, fp);
	fseek(fp, 4, SEEK_SET);
	fread(&(sb.block_size), sizeof(int), 1, fp);
	fseek(fp, 8, SEEK_SET);
	fread(&(sb.offset), sizeof(int), 1, fp);
	fseek(fp, 12, SEEK_SET);
	fread(&(sb.inode_offset), sizeof(int), 1, fp);
	fclose(fp);
}

/* Initializes all values to 0 and NULL */
void loader::initialize(string filename){
	for(int i = 0; i < 256; i++){
		inode_map[i] = 0;
	}
	int end = sb.block_size * sb.num_blocks;
	fbl_block_count = (end - sb.offset)/sb.block_size;
	free_block_list = (int*)malloc(sizeof(int)*fbl_block_count);

	for(int i = 0; i < fbl_block_count; i++){
		free_block_list[i] = 0;
	}

	for(int i = 0; i < 256; i++){	
		inode_mem.push_back(NULL);
	}
}

/* Loads inode bitmap into memory --> inode_map */
void loader::load_inode_map(string filename){
	FILE *fp = fopen(filename.c_str(), "rb");
	fseek(fp, sb.block_size, SEEK_SET);
	bitset<8> bit;
	char c;
	
	int count = 0;
	for(int i = 0; i < 32; i++){
		if(fread(&c, 1, 1, fp) != 1){
			cout << "ERROR" << endl;	
		};
		cout << "test: " << c << endl;
		bit = c;
		cout << "---new byte---" << endl;
	   	for (int j = 7; j >= 0; j--){
			cout << bit[j] << endl;
			inode_map[count] = bit[j];
			count++;
		}
	}
	
}

/* Loads fbl bitmap into memory --> free_block_list */
void loader::load_fbl(string filename){
	FILE *fp = fopen(filename.c_str(), "rb");
	fseek(fp, 2*sb.block_size, SEEK_SET);
	char c;
	int count = 0;
	for(int i = 0; i < fbl_block_count/8 + 1; i++){
		fread(&c, sizeof(c), 1, fp);
		if(count == fbl_block_count){
			break;
		}
	   	for (int j = 7; j >= 0; j--){
			free_block_list[count] = ((c >> j) & 1);
			count++;
		}
	}
	fclose(fp);
}

/* Loads inodes into memory --> inode_mem */
void loader::load_inodes(string filename){
	vector<int> inodes_to_load;
	for(int i = 0; i < 256; i++){
		if(inode_map[i] == 1){
			inodes_to_load.push_back(i);
		}
	}
	if(inodes_to_load.size() != 0){
		inode *node1;
		int num = 0;
		FILE *fp = fopen(filename.c_str(), "rb");
		vector<int>::iterator iter;
		for(iter = inodes_to_load.begin(); iter != inodes_to_load.end(); iter++){
			node1 = new inode();
			num = *iter;
			fseek(fp, sb.offset + num*sb.block_size, SEEK_SET);
			fread(&(node1 -> file_name), sizeof(node1 -> file_name), 1, fp);
			cout << "file name: " << node1 -> file_name << endl;
			fread(&(node1 -> file_size), sizeof(node1 -> file_size), 1, fp);
			cout << "file size: " << node1 -> file_size << endl;
			fread(&(node1 -> total_blocks), sizeof(node1 -> total_blocks), 1, fp);
			cout << "total_blocks: " << node1 -> total_blocks << endl;
			fread(&(node1 -> index), sizeof(node1 -> index), 1, fp);
			cout << "index: " << node1 -> index << endl;
			for(int i = 0; i < 12; i++){
				fread(&(node1 -> direct_ptrs[i]), sizeof(node1 -> file_name), 1, fp);
				cout << "dir ptr: " << i << " " << node1 -> direct_ptrs[i] << endl;
			}
			fread(&(node1 -> indirect_ptrs), sizeof(node1 -> indirect_ptrs), 1, fp);
			cout << "indir ptr: " << node1 -> indirect_ptrs << endl;
			fread(&(node1 -> dindirect_ptrs), sizeof(node1 -> dindirect_ptrs), 1, fp);
			cout << "dindir ptr: " << node1 -> dindirect_ptrs << endl;
			inode_mem.at(node1->index) = node1;
			cout << endl << "values in the indirect block" << endl;
			fseek(fp, (node1 -> indirect_ptrs * sb.block_size), SEEK_SET);
			int indArr[sb.block_size/4];
			for(int i = 0; i < (sb.block_size/4); i++){
				indArr[i] = 0;
			}
			fread(&(indArr), sizeof(int), (sb.block_size/4), fp);
			for(int i = 0; i < (sb.block_size/4); i++){
				cout << indArr[i] << endl;
			}
			cout << endl << "values in the double indirect block" << endl;
			fseek(fp, (node1 -> dindirect_ptrs * sb.block_size), SEEK_SET);
			int iblock = 0;
			fread(&(iblock), sizeof(int), 1, fp);
			cout << "the value in first di " << iblock << endl;
			fseek(fp, (iblock * sb.block_size), SEEK_SET);
			for(int i = 0; i < (sb.block_size/4); i++){
				indArr[i] = 0;
			}
			fread(&(indArr), sizeof(int), (sb.block_size/4), fp);
			for(int i = 0; i < (sb.block_size/4); i++){
				cout << indArr[i] << endl;
			}
		}
	}
}
