//inode.cpp
#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <fstream>
#include <bitset>
#include <sys/stat.h>
#include "global.hpp"
#include "inode.hpp"

using namespace std;

void inode::initialize(string filename){
	
	struct stat st;

	this->file_name =  filename;
	cout << this->file_name << endl;
	
/*	if(stat(file_name.c_str(), &st) != 0){
		fprintf(stderr, "file_size is 0");
		exit(1);
	}*/
	this->file_size = 0;

	//lock here

	int index;
	index = read_free_mem_imap();
	// case where there is not an available free inode
	if(index == -1){
		fprintf(stderr, "no available inodes");
		exit(1);
	}
	//initialize the inode block values to -1;
	for(int i = 0; i < 12; i++){
		this->direct_ptrs[i] = -1;
	}
	this->indirect_ptrs = -1;
	this->dindirect_ptrs = -1;
	//write the inode to memory
	inode_mem.at(index) = this;
	this->index = index;

	update_inode_map(index, 1);

}
















