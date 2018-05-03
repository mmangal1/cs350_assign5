#include "global.hpp"
#include <iostream>
#include <bits/stdc++.h>
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
	if(fp == NULL){
		cout << "exiting" << endl;
		exit(1);
	}
	cout << "sb.inode_offset" << sb.inode_offset << endl;
	fseek(fp, sb.inode_offset, SEEK_SET);
/*	char myChar[33];
	string s = node -> file_name;
	strcpy(myChar, s.c_str());
	fwrite(&(myChar), 32, 1, fp);
*/
	fwrite(&(node -> file_name), 32, 1, fp);
	fwrite(&(node -> file_size), 4, 1, fp);
	fwrite(&(node -> total_blocks), 4, 1, fp);
	fwrite(&(node -> index), 4, 1, fp);
	for(int i = 0; i < 12; i++){
		fwrite(&(node -> direct_ptrs[i]), 4, 1, fp);
	}
	fwrite(&(node -> indirect_ptrs), 4, 1, fp);
	fwrite(&(node -> dindirect_ptrs), 4, 1, fp);
	fclose(fp);
	fp = fopen(disk_name.c_str(), "rb+");
	fseek(fp, sb.inode_offset, SEEK_SET);
	node -> file_name = "not the right file";
	fread(&(node -> file_name), 32, 1, fp);
	cout << "file name: " << node -> file_name << endl;
	fclose(fp);
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

void delete_file(string ssfs_file_name){
	FILE *fp = fopen(disk_name.c_str(), "rb+");
	int inode_index = -1;
	/* find the inode for the given file name*/
	for(int i = 0; i < 256; i++){
		if(inode_mem[i] -> file_name == ssfs_file_name){
			inode_index = i;
			break; 
		}
	}
	if(inode_index == -1){
		fprintf(stderr, "the file you wish to delete does not exist on disk\n");
		exit(1);
	}
	int double_block, freeing_block, indirect_block_index, double_indirect_block_index, num, num_d, store_ip, store_dp;
	double_block = freeing_block = indirect_block_index = double_indirect_block_index = num = num_d = store_ip = store_dp = 0;
	store_ip = inode_mem[inode_index] -> indirect_ptrs;
	store_dp = inode_mem[inode_index] -> dindirect_ptrs;
	if(store_ip != -1){
		update_free_list(store_ip, 0);
	}
	if(store_dp != -1){
		update_free_list(store_dp, 0);
	}
	for(int i = 0; i < 12; i++){
		if(inode_mem[inode_index] -> direct_ptrs[i] == -1){
			delete inode_mem[inode_index];
			update_inode_map(inode_index, 0);
			return;			
		}else{
			update_free_list(inode_mem[inode_index] -> direct_ptrs[i], 0);
		}
	}
	if(inode_mem[inode_index] -> indirect_ptrs == -1){
		return;			
	}else{
		fseek(fp, sb.offset + (inode_mem[inode_index] -> indirect_ptrs)*sb.block_size, SEEK_SET);
		update_free_list(inode_mem[inode_index] -> indirect_ptrs, 0);
		for(int i = 0; i < sb.block_size/4; i++){
			fread(&freeing_block, 4, 1, fp);
			if(freeing_block == -1){
				delete inode_mem[inode_index];
				update_inode_map(inode_index, 0);
				return;					
			}else{
				update_free_list(freeing_block, 0);
			}
		}
	}	
	if(inode_mem[inode_index] -> dindirect_ptrs == -1){
		delete inode_mem[inode_index];
		update_inode_map(inode_index, 0);
		return;
	}else{
		fseek(fp, sb.offset + (inode_mem[inode_index] -> dindirect_ptrs)*sb.block_size, SEEK_SET);
		update_free_list(inode_mem[inode_index] -> dindirect_ptrs, 0);
		for(int i = 0; i < sb.block_size/4; i++){
			fseek(fp, sb.offset + (inode_mem[inode_index] -> dindirect_ptrs)*sb.block_size + i*4, SEEK_SET);
			fread(&freeing_block, 4, 1, fp);
			if(freeing_block == -1){
				delete inode_mem[inode_index];
				update_inode_map(inode_index, 0);
				return;
			}else{
				fseek(fp, sb.offset + freeing_block*sb.block_size, SEEK_SET);
				update_free_list(freeing_block, 0);
				for(int j = 0; j < sb.block_size/4; j++){
					fread(&freeing_block, 4, 1, fp);
					if(freeing_block == -1){
						delete inode_mem[inode_index];
						update_inode_map(inode_index, 0);
						return;							
					}else{
						update_free_list(freeing_block, 0);
					}
				}

			}
		}
	}
	
	fclose(fp);
}

void import(string ssfs_file_name, string unix_file_name){
	
}
