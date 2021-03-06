#include "ssfs_mkdsk.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <bitset>

using namespace std;


ssfs_mkdsk::ssfs_mkdsk(char* file_name, int num_blocks, int block_size){
	/* Creates disk */
	FILE *fp = fopen(file_name, "wb");
	int size = block_size*num_blocks;	
	char *x = new char[size];
	/* Superblock creation
	 * Contains num_blocks and block_size */
	if(fp != NULL){
		//x = new char[size];
		for(int i = 0; i < size; i++){
			x[i] = '\0';
		}
		/* create the file of size block_size*num_blocks */
		fwrite(x, block_size, num_blocks, fp);
	}else{
		fprintf(stderr, "File did not open");
		exit(1);
	}

	/* inode map creation in memory 
	 * Initializing all to unused */
	for(int i = 0; i < 256; i++){
		inode_map[i] = 0;
	}
	/* this is the size of the free block list because the blocks reserved for inodes, inode map, free block list, and super block = 259 */
	double offset = num_blocks-259;
	offset = offset / 8;
	offset = offset / block_size;
	offset = ((int)(offset / 1) +259);
	offset = offset * block_size;
	
	int end = block_size * num_blocks;
//	cout << "block size: " << sb.block_size << " offset: " << sb.offset << endl;
	fbl_block_count = (end - offset)/block_size;

	//fbl_block_count = offset;
	free_block_list = new int[fbl_block_count]; //(int*)malloc(sizeof(int)*fbl_block_count);
	for(int i = 0; i < fbl_block_count; i++){
		free_block_list[i] = 0;
	}
	fclose(fp);
	write_inode_map(inode_map, file_name, block_size, num_blocks);
	write_sb(offset, file_name, block_size, num_blocks);
	write_fbl(free_block_list, file_name, block_size, num_blocks);
	/*write_fbl(free_block_list, file_name, block_size, num_blocks);
	write_sb(offset, file_name, block_size, num_blocks);*/
	delete free_block_list;
	delete x;
}

void ssfs_mkdsk::write_inode_map(int inode_map[], char* file_name, int block_size, int num_blocks){
	FILE *fp = fopen(file_name, "rb+");
	uint8_t currbyte = 0;
	int bitcount = 0;
	int totalcount = 0;
	/* go one block to the origin to write inode bitmap 
	 * Need to write bytes to a file.. so 8 bits at a time*/
	fseek(fp, block_size, SEEK_SET);
	cout << "BS: " << block_size << endl;
	for(int i = 0; i < 256; i++){
		currbyte = (currbyte << 1) | inode_map[i];
		bitcount++;
		if(bitcount == 8){
			totalcount++;
			if(fwrite(&currbyte, 1, 1, fp) != 1){
				cout << "ERROR" << endl;	
			}
			fseek(fp, block_size+totalcount, SEEK_SET);
			currbyte = 0;
			bitcount = 0;
		}
	}
	fclose(fp);/*
	fp = fopen("test.bin", "rb+");
	fseek(fp, block_size, SEEK_SET);
	char c;
	fread(&c, sizeof(char), 1, fp);
	c = c+48;
	cout << c << endl;
	fseek(fp, block_size+1, SEEK_SET);
	fread(&c, sizeof(char), 1, fp);
	c = c+48;
	cout << c << endl;
	fclose(fp);	
	fp = fopen("test.bin", "wb+");*/
}

void ssfs_mkdsk::write_fbl(int free_block_list[], char* file_name, int block_size, int num_blocks){
	FILE *fp = fopen(file_name, "rb+");
	int currbyte = 0;
	int bitcount = 0;
	int totalcount = 0;	
	fseek(fp, 2*block_size, SEEK_SET);
	cout << "BS: " << block_size << endl;

	for(int i = 0; i < fbl_block_count; i++){
		currbyte = (currbyte << 1) | free_block_list[i];
		bitcount++;
		if(bitcount == 8){
			totalcount++;
			fwrite(&currbyte, 1, 1, fp);
			//cout << "currbyte" << (currbyte+48) << endl;
			fseek(fp, 2*block_size+totalcount, SEEK_SET);
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
	fclose(fp);/*
	fp = fopen("test.bin", "rb+");
	fseek(fp, 2*block_size, SEEK_SET);
	char c;
	int test;
	fread(&c, 1, 1, fp);
	test = c;
	cout << test << endl;
	fseek(fp, 2*block_size+1, SEEK_SET);
	fread(&c, 1, 1, fp);
	//c = c+48;
	test = c;
	cout << test << endl;
	fclose(fp);	
	fp = fopen("test.bin", "wb+");*/
}

void ssfs_mkdsk::write_sb(int offset, char* file_name, int block_size, int num_blocks){
	FILE *fp = fopen(file_name, "rb+");
	fseek(fp, 0, SEEK_SET);
	fwrite(&num_blocks, sizeof(int), 1, fp);
	fwrite(&block_size, sizeof(int), 1, fp);
	fwrite(&offset, sizeof(int), 1, fp);
	int inode_offset = offset-(256*block_size);
	fwrite(&inode_offset, sizeof(int), 1, fp);
	fclose(fp);
	cout << "in here " << endl;
//	fprintf(fp, "%d%d%d%d", num_blocks, block_size, offset, (offset - (256 * block_size)));
}
