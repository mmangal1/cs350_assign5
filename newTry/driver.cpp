#include <iostream>
#include <string>
#include <stdlib.h>
#include <sstream>
#include "loader.hpp"
#include "global.hpp"

using namespace std;

int main(int args, char* argv[]){
	
	if(args < 2 || args > 6){
		fprintf(stderr, "invalid usage\n");
		exit(1);
	}else{
		//TODO: fix this
		//ifstream ifile(argv[1]);
		/*if(!ifile){
			fprintf(stderr, "disk file does not exist\n");
		}*/
	}

	
	loader* init = new loader();
	init->load_sb(argv[1]);
	init->initialize(argv[1]);
	init->load_inode_map(argv[1]);
	init->load_fbl(argv[1]);
	//init->load_inodes(argv[1]);
/*	for(int i = 0; i < 256; i++){
		cout << inode_map[i] << endl;
	}
*/
	//delete init;
	return 0;	 	
}
