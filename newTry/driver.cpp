#include <iostream>
#include <string>
#include <stdlib.h>
#include <sstream>
#include <fstream>
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
	disk_name = argv[1];
	
	loader* init = new loader();
	init->load_sb(argv[1]);
	init->initialize(argv[1]);
	init->load_inode_map(argv[1]);
	init->load_fbl(argv[1]);
	init->load_inodes(argv[1]);
	delete init;

	ifstream in(argv[2]);
	if(!in){
		cout << "cannot open input file" << endl;
		exit(1);
	}
	string array[5];
	string str;
	string command;
	array[0] = "";
	array[1] = "";
	array[2] = "";
	array[3] = "";
	array[4] = "";
	int i = 0;
	while(getline(in, str)){
		stringstream ssin(str);
		while(ssin.good() && i < 5){
			ssin >> array[i];
			i++;
		}
		if(array[0] == "CREATE"){
			create(array[1]);
		}
		else if(array[0] == "DELETE"){
			delete_file(array[1]);
		}
	}
	write_inode_map();
	write_fbl();
	return 0;	 	
}
