#include <iostream>
#include <string>
#include <stdlib.h>
#include <sstream>

#include "ssfs_mkdsk.hpp"

using namespace std;

int main(int args, char* argv[]){
	
	if(args != 4){
		fprintf(stderr, "invalid usage\n");
		exit(1);
	}else{
		if(sizeof(argv[3]) > 32){
			fprintf(stderr, "file name must be less than 32 characters\n");
			exit(1);
		}
		if(atoi(argv[1]) < 1024 || atoi(argv[1]) > 128000){
			fprintf(stderr, "num_blocks must be between 1024 and 128000\n");
			exit(1);
		}
		if(atoi(argv[2]) < 128 || atoi(argv[2]) > 512){
			fprintf(stderr, "block_size must be between 128 and 512\n");
			exit(1);
		}
	}
	string num_blocks(argv[1]);
	string block_size(argv[2]);
	int x, y;
	stringstream str(num_blocks);
	str >> x;
	stringstream str1(block_size);
	str1 >> y;
	ssfs_mkdsk *disk = new ssfs_mkdsk(argv[3], x, y);
	delete disk;
	return 0;	 	
}
