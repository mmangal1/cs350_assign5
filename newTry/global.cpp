#include "global.hpp"
#include <iostream>
#include <bits/stdc++.h>
#include <sys/stat.h>

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;
pthread_cond_t full, empty, condition;
queue<shared*> buffer;
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
		//cout << inode_map[i] << " ";
		currbyte = (currbyte << 1) | inode_map[i];
		bitcount++;
		if(bitcount == 8){
			totalcount++;
			fwrite(&currbyte, 1, 1, fp);
			//cout << "currbyte: " << currbyte << endl;
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
	inode *node = new inode;
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
			//if(inode_mem[my_node -> index] == 1){
				if(strcmp(my_node->file_name,ssfs_file_name) == 0){
					fprintf(stderr, "error: file already exists\n");
					return;
				}
			//}
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

	create(unix_file_name);/* this will initialize a node to hold the contents of the file*/
	inode_index = find_inode_index(unix_file_name);

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
	char *newBuff;
	//number if ints an indirect block can hold
	int num_indirect = sb.block_size / 4;
	int count = 0; // determine if we need to move to the next indirect block
	int indirect_block;
	int next = 0;
	int block_val;
	int keeping_count = 0;
	shared *myShared;
	for(int x = 0; x < num_blocks; x++){
		update_free_list(freeBlocks.at(x), 1);
	}
	for(int j = 0; j < num_blocks; j++){
		block_val = (sb.offset + ( freeBlocks.at(j) * sb.block_size));
		//cout << "value in block_val:   " << block_val << endl;
		if(j < 12){
			inode_mem[inode_index]->direct_ptrs[j] = block_val;
		}else if(j >= 12 && j < num_indirect + 13){
			// write the next free block to the indirect block
			if(j == 12){
				inode_mem[inode_index]->indirect_ptrs = block_val;
				// will move to the indirect block to begin writing
			}else{
				//cout << "test: " << inode_mem[inode_index]->indirect_ptrs + keeping_count*4 << endl;
				myShared = set_shared_struct(1, inode_mem[inode_index]->indirect_ptrs + keeping_count*4, newBuff, 4, true, block_val);
				add_to_buffer(myShared);
				while(myShared -> done == false){
					pthread_cond_wait(&(myShared -> condition), &(myShared -> mutex));
				}
				pthread_mutex_unlock(&(myShared -> mutex));
				delete myShared;
	//TODO			//write free blocks into the ptr's block
				// pass to the scheduler thread to write to the disk file
				keeping_count++;
			}
			
		}else{
			if(j == num_indirect + 13){
				//cout << "we got here" << endl;
				inode_mem[inode_index]->dindirect_ptrs = block_val;
			}else{
		//TODO		//write free blocks into the ptr's block
				// pass to the scheduler thread to write to the disk file
				//have to move to next indirect block
				if(count == 0){

					//cout << "test: " << inode_mem[inode_index]->dindirect_ptrs + (sb.block_size * next) << endl;
					myShared = set_shared_struct(1, inode_mem[inode_index]->dindirect_ptrs + (sb.block_size * next), newBuff, 4, true, block_val);
					add_to_buffer(myShared);
					while(myShared -> done == false){
						pthread_cond_wait(&(myShared -> condition), &(myShared -> mutex));
					}
					pthread_mutex_unlock(&(myShared -> mutex));

					delete myShared;
					indirect_block = block_val;
					next++;
				}else{

					//cout << "test: " << block_val + count*4 << endl;
					myShared = set_shared_struct(1, block_val + (count-1)*4, newBuff, 4, true, block_val);
					add_to_buffer(myShared);
					while(myShared -> done == false){
						pthread_cond_wait(&(myShared -> condition), &(myShared -> mutex));
					}
					pthread_mutex_unlock(&(myShared -> mutex));
					delete myShared;
				}
				count++;
				if(count == num_indirect + 1 ) count = 0;

			}	
			
		}
	}
	write_file_to_disk_using_inode_free_blocks(ssfs_file_name, unix_file_name);
}





//HELPER FUNCTION TO WRITE THE CONTENTS OF A FILE TO THE DISK BASED ON THE BLOCK PTRS IN ITS INODE
void write_file_to_disk_using_inode_free_blocks(char ssfs_file_name[], char unix_file_name[]){
	//cout << "inside write_ " << endl;
	int inode_index = -1;
	/* find the inode for the given file name*/
	for(int i = 0; i < 256; i++){
		if(inode_mem[i] != NULL && strcmp(inode_mem[i] -> file_name, unix_file_name) == 0){
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
	int d_indirect_block_index = 0;
	int counter;
/*write data to blocks associated with the inode*/
	FILE* fp = fopen(disk_name.c_str(), "rb+");
	cout << "file: " << unix_file_name << endl;
	FILE* unix_fp = fopen(unix_file_name, "r");
	char buff[sb.block_size];
	char *newBuff;
	shared *myShared;
	for(int i = 0; i < inode_mem[inode_index]->total_blocks; i++){
		if(i < 12){
			newBuff = new char[sb.block_size];
			memset(newBuff, '\0', sb.block_size);
			fseek(unix_fp, sb.block_size*i, SEEK_SET);
			fread(&newBuff, sb.block_size, 1, unix_fp);
			myShared = set_shared_struct(1, inode_mem[inode_index]->direct_ptrs[i], newBuff, sb.block_size, false, -1);
			add_to_buffer(myShared);
			pthread_mutex_lock(&(myShared -> mutex));
			while(myShared -> done == false){
				pthread_cond_wait(&(myShared -> condition), &(myShared -> mutex));
			}
			pthread_mutex_unlock(&(myShared -> mutex));
			delete myShared;
			delete[] newBuff;

			/*
			fseek(fp, inode_mem[inode_index]->direct_ptrs[i], SEEK_SET);
			fseek(unix_fp, sb.block_size*i, SEEK_SET);
			fread(&buff, sb.block_size, 1, unix_fp);
			fwrite(&buff , sb.block_size, 1, fp);
			bzero(&buff, sb.block_size);*/
		}else if(i < (sb.block_size / 4) + 13){/* add 13 to account for the free block that is used to hold the indirect block*/
			if(inode_mem[inode_index]->indirect_ptrs != -1){
				newBuff = new char[sb.block_size];
				memset(newBuff, '\0', sb.block_size);
				myShared = set_shared_struct(0, inode_mem[inode_index]->indirect_ptrs + (indirect_block_index * sizeof(int)), newBuff, 4, true, -1);
				add_to_buffer(myShared);
				while(myShared -> done == false){
					pthread_cond_wait(&(myShared -> condition), &(myShared -> mutex));
				}
				pthread_mutex_unlock(&(myShared -> mutex));
				freeing_block = myShared -> myInt;
				delete myShared;

				fread(&newBuff, sb.block_size, 1, unix_fp);
				


				delete newBuff;
				/*
				fseek(fp, inode_mem[inode_index]->indirect_ptrs + (indirect_block_index * sizeof(int)), SEEK_SET);
				fread(&(freeing_block), sizeof(freeing_block), 1, fp);
				fseek(fp, freeing_block, SEEK_SET);//
				fread(&buff, sb.block_size, 1, unix_fp);
				fwrite(&buff, sb.block_size, 1, fp);
				indirect_block_index++;*/ /* increment the position in the indirect block*/
			}else break;
		}else{
			if(inode_mem[inode_index]->dindirect_ptrs != -1){
				fseek(fp, inode_mem[inode_index]->dindirect_ptrs + (d_indirect_block_index * sizeof(int)), SEEK_SET);/*move to index in dindirect block*/
				fread(&(double_block), sizeof(double_block), 1, fp);/*read indirect block ptr*/
				fseek(fp, double_block + (double_indirect_block_index * sizeof(int)), SEEK_SET);/* move index in indirect block*/
				fread(&(freeing_block), sizeof(freeing_block), 1, fp);/* read block number to free in the direct block*/
				fseek(fp, freeing_block, SEEK_SET);
				fread(&buff, sb.block_size, 1, unix_fp);
				fwrite(&buff, sb.block_size, 1, fp);
				double_indirect_block_index++; /*move to next index in indirect block*/
				if(double_indirect_block_index == sb.block_size /4){ /*if we have read an entire indirect block*/
					double_indirect_block_index = 0; /*prepare to read first int in the next indirect block*/
					d_indirect_block_index++; /*move to next index in double indirect block*/
					free_block_list[double_block] = 0; /* free indirect block that we have completely read*/
				}
			}else break;
			//if(i == (block_size / 4) + 11) free_block_list[inode_map[inode_index].indirect_ptrs = 0; /* free the indirect_ptrs block*/
		}
		bzero(&buff, sb.block_size);
	}
/* all blocks should be written*/

	fclose(fp);
}


/*HELPER FUNCTION TO DETERMINE THE IN0DES OF A GIVEN FILE, RETURN -1 IF NOT ON DISK*/
int find_inode_index(char ssfs_file_name[]){
	int inode_index = -1;
	/* find the inode for the given file name*/
	for(int i = 0; i < 256; i++){
		if(inode_map[i] == 1){
			if(inode_mem[i] != NULL && strcmp(inode_mem[i] -> file_name, ssfs_file_name) == 0){
				inode_index = i;
				return inode_index;
			}
		}
	}
	return inode_index;
}



/*
CAT <SSFS file name>
This	command	displays	the	contents	of	<SSFS file name> on	the	screen,	just	like	the	unix	cat	command.
*/
void cat(char ssfs_file_name[]){
	//cout << "entering cat function" << endl;

	int inode_index = find_inode_index(ssfs_file_name);
	if(inode_index == -1){
		fprintf(stderr, "the file you wish to cat does not have an inode that exists on disk");
		exit(1);
	}


	/* look at file size blocks and free them*/
	//int double_count;
	int double_block;
	int freeing_block;
	int indirect_block_index = 0;
	int double_indirect_block_index = 0;
	char buff[sb.block_size];
	char *newBuff;
	shared *myShared;
	cout << "total blocks in the file we cat  " << inode_mem[inode_index]->total_blocks << endl;
	for(int i = 0; i < inode_mem[inode_index]->total_blocks; i++){
		if(i < 12){
			newBuff = new char[sb.block_size];
			memset(newBuff, '\0', sb.block_size);
			myShared = set_shared_struct(0, inode_mem[inode_index]->direct_ptrs[i], newBuff, sb.block_size, false, -1);
			add_to_buffer(myShared);
			pthread_mutex_lock(&(myShared -> mutex));
			while(myShared -> done == false){
				pthread_cond_wait(&(myShared -> condition), &(myShared -> mutex));
			}
			pthread_mutex_unlock(&(myShared -> mutex));
			cout.write(myShared -> data, sb.block_size);
			delete myShared;
			delete[] newBuff;
		}else if(i < (sb.block_size / 4) + 12){// add 12 ??TODO check that this is enough iterations
			if(inode_mem[inode_index]->indirect_ptrs != -1){
				newBuff = new char[5];
				memset(newBuff, '\0', 5);
				myShared = set_shared_struct(0, inode_mem[inode_index]->indirect_ptrs + (indirect_block_index*sizeof(int)), newBuff, 4, true, -1);
				add_to_buffer(myShared);
				while(myShared -> done == false){
					pthread_cond_wait(&(myShared -> condition), &(myShared -> mutex));
				}
				pthread_mutex_unlock(&(myShared -> mutex));
				freeing_block = myShared -> myInt;
				delete myShared;
				delete newBuff;
				newBuff = new char[sb.block_size];
				memset(newBuff, '\0', sb.block_size);
				myShared = set_shared_struct(0, freeing_block, newBuff, sb.block_size, false, -1);
				add_to_buffer(myShared);
				while(myShared -> done == false){
					pthread_cond_wait(&(myShared -> condition), &(myShared -> mutex));
				}
				pthread_mutex_unlock(&(myShared -> mutex));
				cout.write(myShared -> data, sb.block_size);
				indirect_block_index++;
				delete myShared;
				delete[] newBuff;
			}else break;
			if(i == ( (sb.block_size / 4) + 12 ) - 1){
				indirect_block_index = 0;
			}
		}else{
			if(inode_mem[inode_index]->dindirect_ptrs != -1){

				newBuff = new char[5];
				memset(newBuff, '\0', 5);
				myShared = set_shared_struct(0, inode_mem[inode_index]->dindirect_ptrs + (indirect_block_index * sizeof(int)), newBuff, 4, true, -1);
				add_to_buffer(myShared);
				while(myShared -> done == false){
					pthread_cond_wait(&(myShared -> condition), &(myShared -> mutex));
				}
				pthread_mutex_unlock(&(myShared -> mutex));
				double_block = myShared -> myInt;
				delete myShared;
				delete newBuff;

				newBuff = new char[5];
				memset(newBuff, '\0', 5);
				myShared = set_shared_struct(0, double_block + (double_indirect_block_index * sizeof(int)), newBuff, 4, true, -1);
				add_to_buffer(myShared);
				while(myShared -> done == false){
					pthread_cond_wait(&(myShared -> condition), &(myShared -> mutex));
				}
				pthread_mutex_unlock(&(myShared -> mutex));
				freeing_block = myShared -> myInt;
				delete myShared;
				delete newBuff;

				newBuff = new char[sb.block_size];
				memset(newBuff, '\0', sb.block_size);
				myShared = set_shared_struct(0, freeing_block, newBuff, sb.block_size, false, -1);
				add_to_buffer(myShared);
				while(myShared -> done == false){
					pthread_cond_wait(&(myShared -> condition), &(myShared -> mutex));
				}
				pthread_mutex_unlock(&(myShared -> mutex));
				cout.write(myShared -> data, sb.block_size);
				double_indirect_block_index++;
				delete myShared;
				delete[] newBuff;
/*

				fseek(fp, inode_mem[inode_index]->dindirect_ptrs + (indirect_block_index * sizeof(int)), SEEK_SET);//move to index in dindirect block
				//cout << 
				fread(&(double_block), sizeof(double_block), 1, fp);//read indirect block ptr
				//cout << "value of double block in double indirect ptr:   " << double_block << endl;
				fseek(fp, double_block + (double_indirect_block_index * sizeof(int)), SEEK_SET);// move index in indirect block
				fread(&(freeing_block), sizeof(freeing_block), 1, fp);// read block number to free in the direct block
				fseek(fp, freeing_block, SEEK_SET);
				//cout << "value of freeing block in double indirect ptr:   " << freeing_block << endl;
				fread(&buff, sb.block_size, 1, fp);
				cout << buff << endl;
				double_indirect_block_index++; //move to next index in indirect block*/
				if(double_indirect_block_index == sb.block_size /4){ //if we have read an entire indirect block
					double_indirect_block_index = 0; //prepare to read first int in the next indirect block
					indirect_block_index++; //move to next index in double indirect block
					//free_block_list[double_block] = 0; // free indirect block that we have completely read
				}
			}else break;
			//if(i == (block_size / 4) + 11) free_block_list[inode_map[inode_index].indirect_ptrs = 0; // free the indirect_ptrs block
		}
		//bzero(&buff, sb.block_size);
	}

/* all blocks should be written*/

cout << endl;
	//fclose(fp);



}





void read(char ssfs_file_name[], int start_byte, int num_bytes){
	cout << "inside read" << endl;
	int inode_index = find_inode_index(ssfs_file_name);
	if(inode_index == -1){
		fprintf(stderr, "the file you wish to cat does not have an inode that exists on disk");
		exit(1);
	}
// find which block will hold the start byte (start_byte / block_size)
	int temp = 0;
	int position_in_block = start_byte % sb.block_size;
	int block_to_look_for_in_inode = (start_byte / sb.block_size);
	int seek_position;
	int temp_start_byte = start_byte;
	char output;	
	cout << "down here inside read" << endl;
	while(temp < num_bytes){
		block_to_look_for_in_inode = (temp_start_byte / sb.block_size);
		if(block_to_look_for_in_inode == 0){
			block_to_look_for_in_inode = 1;
		}
		seek_position = get_ptr_to_the_block_we_need_to_read(inode_index, block_to_look_for_in_inode);
		position_in_block = temp_start_byte % sb.block_size;
		if(seek_position == -1){
			fprintf(stderr, "was unable to locate position of start_byte");
			exit(1);
		}
	cout << "before file inside read" << endl;
		FILE* fp = fopen(disk_name.c_str(), "rb+");
		while(position_in_block < sb.block_size){
			fseek(fp, seek_position + position_in_block, SEEK_SET);
//TODO			fread(&(output), 1, 1, fp);
			cout << output;
			temp++;
			position_in_block++;
		}
		temp_start_byte += temp;
		position_in_block = 0;

	//find position to start reading in a block is start_byte % block_size


		fclose(fp);
	}
	cout << endl;


}




int get_ptr_to_the_block_we_need_to_read(int inode_index, int find_block){


	int temp = -1;
	// look at file size blocks and free them
	//int double_count;
	int double_block;
	int freeing_block;
	int indirect_block_index = 0;
	int double_indirect_block_index = 0;
//write data to blocks associated with the inode
	FILE* fp = fopen(disk_name.c_str(), "rb+");
	char buff[sb.block_size];
//cout << "this far in the cat function" << endl;
	//for(int i = 0; i < find_block; i++){ changed this line
	for(int i = 0; i < find_block; i++){
		//bzero(&buff, sb.block_size);
		if(i < 12){
			//cout << "dir ptr: " << inode_mem[inode_index]->direct_ptrs[i] << endl;
			//fseek(fp, sb.offset + sb.block_size*inode_mem[inode_index]->direct_ptrs[i], SEEK_SET);
			temp = inode_mem[inode_index]->direct_ptrs[i];
			//cout << "contents of the cat buffer   " <<  buff << endl;
			
			//bzero(&buff, sb.block_size);
		}else if(i < (sb.block_size / 4) + 12){// add 12 ??TODO check that this is enough iterations
			if(inode_mem[inode_index]->indirect_ptrs != -1){
				fseek(fp, inode_mem[inode_index]->indirect_ptrs + (indirect_block_index * sizeof(int)), SEEK_SET);
				fread(&(freeing_block), sizeof(freeing_block), 1, fp);
				
				temp = freeing_block;
				indirect_block_index++; // increment the position in the indirect block
			}else break;
			if(i == ( (sb.block_size / 4) + 12 ) - 1){
				indirect_block_index = 0;
			}
		}else{
			//cout << " in the double indirect block " << endl;
			if(inode_mem[inode_index]->dindirect_ptrs != -1){
				fseek(fp, inode_mem[inode_index]->dindirect_ptrs + (indirect_block_index * sizeof(int)), SEEK_SET);//move to index in dindirect block
				//cout << 
				fread(&(double_block), sizeof(double_block), 1, fp);//read indirect block ptr
				//cout << "value of double block in double indirect ptr:   " << double_block << endl;
				fseek(fp, double_block + (double_indirect_block_index * sizeof(int)), SEEK_SET);// move index in indirect block
				fread(&(freeing_block), sizeof(freeing_block), 1, fp);// read block number to free in the direct block
				
				
				temp = freeing_block;
				double_indirect_block_index++; //move to next index in indirect block
				if(double_indirect_block_index == sb.block_size /4){ //if we have read an entire indirect block
					double_indirect_block_index = 0; //prepare to read first int in the next indirect block
					indirect_block_index++; //move to next index in double indirect block
					//free_block_list[double_block] = 0; // free indirect block that we have completely read
				}
			}else break;
			//if(i == (block_size / 4) + 11) free_block_list[inode_map[inode_index].indirect_ptrs = 0; // free the indirect_ptrs block
		}

		//bzero(&buff, sb.block_size);
	}

// all blocks should be written

	fclose(fp);

	return temp;

}


void add_to_buffer(shared *myShared){
	pthread_mutex_lock(&mutex2);
	if(buffer.size() == 8){
		pthread_cond_wait(&empty, &mutex2);
	}
	buffer.push(myShared);
	pthread_cond_signal(&full);
				
	pthread_mutex_unlock(&mutex2);
}

shared* set_shared_struct(int operation, int block_num, char *data, int size, bool readInt, int block_val){
	shared *myShared = new shared;
	myShared -> operation = operation;
	myShared -> block_num = block_num;
	myShared -> data = data;
	myShared -> done = false;
	myShared -> mutex = PTHREAD_MUTEX_INITIALIZER;
	myShared -> condition = PTHREAD_COND_INITIALIZER;
	myShared -> size = size;
	myShared -> myInt = block_val;
	myShared -> readInt = readInt;
	return myShared;
}




