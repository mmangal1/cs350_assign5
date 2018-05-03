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
void loader::load_sb(char* filename){
	FILE *fp = fopen(filename, "rb");
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
void loader::initialize(char* filename){
	for(int i = 0; i < 256; i++){
		inode_map[i] = 0;
	}
	int end = sb.block_size * sb.num_blocks;
	cout << "block size: " << sb.block_size << " offset: " << sb.offset << endl;
	fbl_block_count = (end - sb.offset)/sb.block_size;
	free_block_list = new int[fbl_block_count+1];//(int*)malloc(sizeof(int)*fbl_block_count);

	for(int i = 0; i < fbl_block_count; i++){
		free_block_list[i] = 0;
	}

	for(int i = 0; i < 256; i++){	
		inode_mem.push_back(NULL);
	}
}

/* Loads inode bitmap into memory --> inode_map */
void loader::load_inode_map(char* filename){
	FILE *fp = fopen(filename, "rb");
	fseek(fp, sb.block_size, SEEK_SET);
	bitset<8> bit;
	char c;
	int count = 0;
	for(int i = 0; i < 32; i++){
		fread(&c, 1, 1, fp);
		bit = c;
	   	for (int j = 7; j >= 0; j--){
			inode_map[count] = bit[j];
			count++;
		}
	}
	fclose(fp);
}

/* Loads fbl bitmap into memory --> free_block_list */
void loader::load_fbl(char* filename){
	FILE *fp = fopen(filename, "rb");
	fseek(fp, 2*sb.block_size, SEEK_SET);
	bitset<8> bit;
	char c;
	int count = 0;
	for(int i = 0; i < fbl_block_count/8 + 1; i++){
		fread(&c, sizeof(char), 1, fp);
		if(count == fbl_block_count){
			break;
		}
		bit = c;
	   	for (int j = 7; j >= 0; j--){
			if(count == fbl_block_count){
				fclose(fp);
				return;
			}
			free_block_list[count] = bit[j];
			count++;
		}
	}
	fclose(fp);
}

/* Loads inodes into memory --> inode_mem */
void loader::load_inodes(char *filename){
	vector<int> inodes_to_load;
	for(int i = 0; i < 256; i++){
		if(inode_map[i] == 1){
			cout << "found one: " << i << endl;
			inodes_to_load.push_back(i);
		}
	}
	if(inodes_to_load.size() != 0){
		inode *node1;
		int num = 0;
		FILE *fp = fopen(disk_name.c_str(), "rb+");
		vector<int>::iterator iter;
		for(iter = inodes_to_load.begin(); iter != inodes_to_load.end(); iter++){
			node1 = (inode*)malloc(sizeof(*node1));
			num = *iter;
			fseek(fp, sb.inode_offset+num*sb.block_size, SEEK_SET);
			//node1 -> file_name = "f1.txt";
			fread(&(node1 -> file_name), 32, 1, fp);
			cout << "file name: " << node1 -> file_name << endl;
			fread(&(node1 -> file_size), 4, 1, fp);
			cout << "file size: " << node1 -> file_size << endl;
			fread(&(node1 -> total_blocks), 4, 1, fp);
			cout << "total_blocks: " << node1 -> total_blocks << endl;
			fread(&(node1 -> index), 4, 1, fp);
			cout << "index: " << node1 -> index << endl;
			for(int i = 0; i < 12; i++){
				fread(&(node1 -> direct_ptrs[i]), 4, 1, fp);
				cout << "dir ptr: " << i << " " << node1 -> direct_ptrs[i] << endl;
			}
			fread(&(node1 -> indirect_ptrs), 4, 1, fp);
			cout << "indir ptr: " << node1 -> indirect_ptrs << endl;
			fread(&(node1 -> dindirect_ptrs), 4, 1, fp);
			cout << "dindir ptr: " << node1 -> dindirect_ptrs << endl;
			inode_mem.at(node1->index) = node1;
			cout << endl << "values in the indirect block" << endl;
			fseek(fp, (node1 -> indirect_ptrs * sb.block_size), SEEK_SET);
			int indArr[sb.block_size/4];
			for(int i = 0; i < (sb.block_size/4); i++){
				indArr[i] = 0;
			}
			fread(&(indArr), 4, (sb.block_size/4), fp);
			for(int i = 0; i < (sb.block_size/4); i++){
				cout << indArr[i] << endl;
			}
			cout << endl << "values in the double indirect block" << endl;
			fseek(fp, (node1 -> dindirect_ptrs * sb.block_size), SEEK_SET);
			int iblock = 0;
			fread(&(iblock), 4, 1, fp);
			cout << "the value in first di " << iblock << endl;
			fseek(fp, (iblock * sb.block_size), SEEK_SET);
			for(int i = 0; i < (sb.block_size/4); i++){
				indArr[i] = 0;
			}
			fread(&(indArr), 4, (sb.block_size/4), fp);
			for(int i = 0; i < (sb.block_size/4); i++){
				cout << indArr[i] << endl;
			}
		}
		fclose(fp);
	}
}
