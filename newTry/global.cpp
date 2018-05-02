#include "global.hpp"
#include <iostream>
superblock sb;
int inode_map[256];
vector<inode*> inode_mem;
int * free_block_list;
int fbl_block_count;
string disk_name;

int read_free_mem_imap(){
	for(int i = 0; i < sizeof(inode_map)/sizeof(inode_map[0]); i++){
		if(inode_map[i] == 0){
			//cout << i << endl;
			return i;
		}
	}
	return -1;
}

vector<int> read_fbl(){
	vector<int> ret_val;
	for(int i = 0; i < fbl_block_count; i++){//make the loop 'free_block_count' times
		if(free_block_list[i] == 0){
			ret_val.push_back(sb.offset+(sb.block_size * i));
		}
	}
	return ret_val;
}

void update_inode_map(int index, int bit_to_set){
	//lock 
	inode_map[index] = bit_to_set;
	//unlock
}

void update_free_list(int index, int bit_to_set){
	//lock
	free_block_list[index] = bit_to_set;
	//unlock
}

void write_fbl(){
	FILE *fp = fopen(disk_name.c_str(), "rb+");
	cout << "disk: " << disk_name << endl;
	int currbyte = 0;
	int bitcount = 0;
	int totalcount = 0;	
	fseek(fp, 2*sb.block_size, SEEK_SET);

	for(int i = 0; i < fbl_block_count; i++){
		currbyte = (currbyte << 1) | free_block_list[i];
		bitcount++;
		if(bitcount == 8){
			totalcount++;
			fwrite(&currbyte, 1, 1, fp);
			fseek(fp, 2*sb.block_size+totalcount, SEEK_SET);
			currbyte = 0;
			bitcount = 0;
		}
	}
	if(bitcount != 0){
		while(bitcount != 8){
			currbyte = (currbyte << 1) | 1;
			bitcount++;
		}
		totalcount++;
		fwrite(&currbyte, 1, 1, fp);
	}
	fclose(fp);
}

void write_inode_map(){
	FILE *fp = fopen(disk_name.c_str(), "rb+");
	if(fp == 0){
		cout << "error opening file" << endl;
	}
	uint8_t currbyte = 0;
	int bitcount = 0;
	int totalcount = 0;
	/* go one block to the origin to write inode bitmap 
	 * Need to write bytes to a file.. so 8 bits at a time*/
	fseek(fp, sb.block_size, SEEK_SET);
	for(int i = 0; i < 256; i++){
		cout << inode_map[i] << " ";
		currbyte = (currbyte << 1) | inode_map[i];
		bitcount++;
		if(bitcount == 8){
			totalcount++;
			fwrite(&currbyte, 1, 1, fp);
			cout << "currbyte: " << currbyte << endl;
			fseek(fp, sb.block_size+totalcount, SEEK_SET);
			currbyte = 0;
			bitcount = 0;
		}
	}
	fclose(fp);
}

void write_inode_to_disk(inode *node){
	FILE *fp = fopen(disk_name.c_str(), "rb+");
	fseek(fp, sb.offset, SEEK_SET);
	fwrite(&(node -> file_name), sizeof(node -> file_name), 1, fp);
	fwrite(&(node -> file_size), sizeof(node -> file_size), 1, fp);
	for(int i = 0; i < 12; i++){
		fwrite(&(node -> direct_ptrs[i]), sizeof(node -> file_name), 1, fp);
	}
	fwrite(&(node -> indirect_ptrs), sizeof(node -> indirect_ptrs), 1, fp);
	fwrite(&(node -> dindirect_ptrs), sizeof(node -> dindirect_ptrs), 1, fp);
}



/* checks to see if file already exists. if not, it initializes a new inode */
void create(string ssfs_file_name){
	vector<inode*>::iterator iter;
	inode *my_node;
	for(iter = inode_mem.begin(); iter != inode_mem.end(); iter++){
		my_node = *iter;
		if(my_node != NULL){
			if(my_node->file_name == ssfs_file_name){
				fprintf(stderr, "error: file already exists\n");
				return;
			}
		}	
	}
	if(read_free_mem_imap() == -1){
		fprintf(stderr, "error: not enough space for a new inode\n");
		return;
	}else{
		inode* node = new inode();
		node -> initialize(ssfs_file_name);
		write_inode_map();
		write_inode_to_disk(node);
	}
}


void import(string ssfs_file_name, string unix_file_name){
	
}
