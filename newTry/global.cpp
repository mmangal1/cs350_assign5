#include "global.hpp"
#include <iostream>
#include <bits/stdc++.h>
#include <sys/stat.h>
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
			ret_val.push_back(i);
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

void write_inode_to_disk(inode *node, int index){
	FILE *fp = fopen(disk_name.c_str(), "rb+");
	if(fp == NULL){
		cout << "exiting" << endl;
		exit(1);
	}
	fseek(fp, sb.inode_offset + sb.block_size*index, SEEK_SET);
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
}

void initialize_inode(char filename[]){
	inode *node = (inode*)malloc(sizeof(*node));
	int index = read_free_mem_imap();
	if(index == -1){
		fprintf(stderr, "no available inodes\n");
		exit(1);
	}
	int len = strlen(filename);
	int i;
	for(i = 0; i < len; i++){
		node->file_name[i] = filename[i];
	}
	for(int j = i; i < 32; i++){
		node->file_name[j] = '\0';
	}
	node->file_size = 0;
	node->total_blocks = 0;
	node->index = index;
	for(int i = 0; i < 12; i++){
		node->direct_ptrs[i] = -1;
	}
	node->indirect_ptrs = -1;
	node->dindirect_ptrs = -1;
	inode_mem[index] = node;
	update_inode_map(index, 1);
	write_inode_to_disk(node, index);
}

/* checks to see if file already exists. if not, it initializes a new inode */
void create(char ssfs_file_name[]){
	vector<inode*>::iterator iter;
	inode *my_node;
	for(iter = inode_mem.begin(); iter != inode_mem.end(); iter++){
		my_node = *iter;
		if(my_node != NULL){
			if(strcmp(my_node->file_name,ssfs_file_name) == 0){
				fprintf(stderr, "error: file already exists\n");
				return;
			}
		}	
	}
	if(read_free_mem_imap() == -1){
		fprintf(stderr, "error: not enough space for a new inode\n");
		return;
	}else{
		initialize_inode(ssfs_file_name);
		write_inode_map();
		//write_inode_to_disk(node);
	}
}

void delete_file(char ssfs_file_name[]){
	FILE *fp = fopen(disk_name.c_str(), "rb+");
	int inode_index = -1;
	/* find the inode for the given file name*/
	for(int i = 0; i < 256; i++){
		if(inode_mem[i] != NULL && strcmp(inode_mem[i] -> file_name, ssfs_file_name) == 0){
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
			inode_mem[inode_index] = NULL;
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
				inode_mem[inode_index] = NULL;
				update_inode_map(inode_index, 0);
				return;					
			}else{
				update_free_list(freeing_block, 0);
			}
		}
	}	
	if(inode_mem[inode_index] -> dindirect_ptrs == -1){
		delete inode_mem[inode_index];
		inode_mem[inode_index] = NULL;
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
				inode_mem[inode_index] = NULL;
				update_inode_map(inode_index, 0);
				return;
			}else{
				fseek(fp, sb.offset + freeing_block*sb.block_size, SEEK_SET);
				update_free_list(freeing_block, 0);
				for(int j = 0; j < sb.block_size/4; j++){
					fread(&freeing_block, 4, 1, fp);
					if(freeing_block == -1){
						delete inode_mem[inode_index];
						inode_mem[inode_index] = NULL;
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

void list(){
	for(int i = 0; i < 256; i++){
		if(inode_map[i] == 1){
			cout << "name: " << inode_mem[i] -> file_name << " size: " << inode_mem[i] -> file_size << endl;
		}
	}
}

void import(char ssfs_file_name[], char unix_file_name[]){
	int inode_index = find_inode_index(ssfs_file_name);

	if(inode_index != -1){
		delete_file(ssfs_file_name);
	}

	create(ssfs_file_name);/* this will initialize a node to hold the contents of the file*/
	inode_index = find_inode_index(ssfs_file_name);


	struct stat st;	
	if(stat(unix_file_name, &st) != 0){
		fprintf(stderr, "file_size is 0");
		exit(1);
	}
	int file_size = st.st_size;
	inode_mem[inode_index] -> file_size = file_size;
	inode_mem[inode_index] -> index = inode_index;



	//number of blocks needed to store the file
	int num_blocks;
	if(file_size < sb.block_size){
		num_blocks = 1;
	}else if (file_size % sb.block_size == 0){
		num_blocks = (file_size / sb.block_size);
	}else{
		num_blocks = (file_size / sb.block_size) + 1;
	}
	inode_mem[inode_index] -> total_blocks = num_blocks;
	//vector to hold free blocks we will be giving to the inode
	vector<int> freeBlocks = read_fbl();

	//figure out some algorithm to determine how many additional blocks are needed for the indirect ptrs
	if(num_blocks > 12 && num_blocks <= 12 + (sb.block_size / 4)){
		num_blocks++;  /* need an extra block for the indirect block*/
	}else if(num_blocks > 12 + (sb.block_size / 4)){/*need double indirect ptrs*/
		int temp = num_blocks;
		temp = temp - (12 + (sb.block_size / 4));
		while(temp > 0){
			num_blocks++;
			temp = temp -(sb.block_size / 4);
		}
		num_blocks++; /*need a block for the indirect ptr*/
		
	}

	if(freeBlocks.size() < num_blocks){
		//do not conduct the write
		fprintf(stderr, "not enough available free blocks");
		exit(1);
	}
/*write blocks to the disk that the inode needs to store the file*/
//need to open the disk file for writing
	FILE* fp = fopen(disk_name.c_str(), "rb+");

	//number if ints an indirect block can hold
	int num_indirect = sb.block_size / 4;
	cout << "num_indirect = " << num_indirect << endl;
	int count = 0; // determine if we need to move to the next indirect block
	int indirect_block;
	int next = 0;
	for(int j = 0; j < num_blocks; j++){
		if(j < 12){
			inode_mem[inode_index]->direct_ptrs[j] = freeBlocks.at(j);
		}else if(j >= 12 && j < num_indirect + 13){
			// write the next free block to the indirect block
			if(j == 12){
				inode_mem[inode_index]->indirect_ptrs = freeBlocks.at(j);
				// will move to the indirect block to begin writing
				fseek(fp, (inode_mem[inode_index]->indirect_ptrs * sb.block_size), SEEK_SET);
			}else{
	//TODO			//write free blocks into the ptr's block
				// pass to the scheduler thread to write to the disk file
				fwrite(&freeBlocks.at(j), sizeof(int), 1, fp);
			}
			
		}else{
			if(j == num_indirect + 13){
				cout << "we got here" << endl;
				inode_mem[inode_index]->dindirect_ptrs = freeBlocks.at(j);
				// will move to the indirect block to begin writing
				fseek(fp, (inode_mem[inode_index]->dindirect_ptrs * sb.block_size), SEEK_SET);
			}else{
		//TODO		//write free blocks into the ptr's block
				// pass to the scheduler thread to write to the disk file
				//have to move to next indirect block
				if(count == 0/*count % num_indirect == 0*/){
				//cout << " in here???" << endl;
					fseek(fp, (inode_mem[inode_index]->dindirect_ptrs * sb.block_size) + (sb.block_size * next), SEEK_SET);
					fwrite(&freeBlocks.at(j), sizeof(int), 1, fp);
					indirect_block = freeBlocks.at(j);
					fseek(fp, (freeBlocks.at(j) * sb.block_size), SEEK_SET);
					next++;
				}else{
					//cout << freeBlocks.at(j) << "double indirect " << endl;
					fwrite(&freeBlocks.at(j), sizeof(int), 1, fp);
					//count++;
				}
				count++;
				if(count ==num_indirect+1) count = 0;

			}	
			
		}
	}
/* finished writing the free blocks into the inode*/


/* write the data of the unix file by going through the inode and writing a block at a time based on the ptrs*/
	write_file_to_disk_using_inode_free_blocks(ssfs_file_name, unix_file_name);
	fclose(fp);//close the disk file
}





//HELPER FUNCTION TO WRITE THE CONTENTS OF A FILE TO THE DISK BASED ON THE BLOCK PTRS IN ITS INODE
void write_file_to_disk_using_inode_free_blocks(char ssfs_file_name[], char unix_file_name[]){
	int inode_index = -1;
	/* find the inode for the given file name*/
	for(int i = 0; i < 256; i++){
		if(inode_mem[i] != NULL && strcmp(inode_mem[i] -> file_name, ssfs_file_name) == 0){
			cout << "found one: " << i << endl;
			inode_index = i;
			break; 
		}
	}
	if(inode_index == -1){
		fprintf(stderr, "the file you wish to import does not have an inode that exists on disk");
		exit(1);
	}
	/* look at file size blocks and free them*/
	//int double_count;
	int double_block;
	int freeing_block;
	int indirect_block_index = 0;
	int double_indirect_block_index = 0;
/*write data to blocks associated with the inode*/
	FILE* fp = fopen(disk_name.c_str(), "rb+");
	cout << "file: " << unix_file_name << endl;
	FILE* unix_fp = fopen(unix_file_name, "r");
	char buff[sb.block_size];
	//for(int i = 0; i < inode_map[inode_index].file_size; i++){ changed this line
	for(int i = 0; i < inode_mem[inode_index]->total_blocks; i++){
		if(i < 12){
			cout << "dir ptr: " << inode_mem[inode_index]->direct_ptrs[i] << endl;
			fseek(fp, sb.offset + sb.block_size*inode_mem[inode_index]->direct_ptrs[i], SEEK_SET);
			fseek(unix_fp, sb.block_size*i, SEEK_SET);
			fread(&buff, sb.block_size, 1, unix_fp);
			fwrite(&buff, sb.block_size, 1, fp);
		}else if(i < (sb.block_size / 4) + 13){/* add 13 to account for the free block that is used to hold the indirect block*/
			if(inode_mem[inode_index]->indirect_ptrs != -1){
				fseek(fp, inode_mem[inode_index]->indirect_ptrs + (indirect_block_index * sizeof(int)), SEEK_SET);
				fread(&(freeing_block), sizeof(freeing_block), 1, fp);
				fseek(fp, freeing_block, SEEK_SET);
				fread(&buff, sb.block_size, 1, unix_fp);
				fwrite(&buff, sb.block_size, 1, fp);
				indirect_block_index++; /* increment the position in the indirect block*/
			}else break;
			if(i == (sb.block_size / 4) + 12){
				indirect_block_index = 0;
			}
		}else{
			if(inode_mem[inode_index]->dindirect_ptrs != -1){
				fseek(fp, inode_mem[inode_index]->dindirect_ptrs + (indirect_block_index * sizeof(int)), SEEK_SET);/*move to index in dindirect block*/
				fread(&(double_block), sizeof(double_block), 1, fp);/*read indirect block ptr*/
				fseek(fp, double_block + (double_indirect_block_index * sizeof(int)), SEEK_SET);/* move index in indirect block*/
				fread(&(freeing_block), sizeof(freeing_block), 1, fp);/* read block number to free in the direct block*/
				fseek(fp, freeing_block, SEEK_SET);
				fread(&buff, sb.block_size, 1, unix_fp);
				fwrite(&buff, sb.block_size, 1, fp);
				double_indirect_block_index++; /*move to next index in indirect block*/
				if(double_indirect_block_index == sb.block_size /4){ /*if we have read an entire indirect block*/
					double_indirect_block_index = 0; /*prepare to read first int in the next indirect block*/
					indirect_block_index++; /*move to next index in double indirect block*/
					free_block_list[double_block] = 0; /* free indirect block that we have completely read*/
				}
			}else break;
			//if(i == (block_size / 4) + 11) free_block_list[inode_map[inode_index].indirect_ptrs = 0; /* free the indirect_ptrs block*/
		}
	}
/* all blocks should be written*/


}









/*HELPER FUNCTION TO DETERMINE THE INDES OF A GIVEN FILE, RETURN -1 IF NOT ON DISK*/
int find_inode_index(char ssfs_file_name[]){
	int inode_index = -1;
	/* find the inode for the given file name*/
	for(int i = 0; i < 256; i++){
		if(inode_mem[i] != NULL && strcmp(inode_mem[i] -> file_name, ssfs_file_name) == 0){
			inode_index = i;
			return inode_index;
		}
	}
	return inode_index;
}
