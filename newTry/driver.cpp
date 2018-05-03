#include <iostream>
#include <string>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <cstring>
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

	/*if(args == 3){
		char test[8] = "foo.txt";
		create(test);
		char test1[9] = "foo1.txt";
		create(test1);
		char test2[9] = "foo2.txt";
		create(test2);
		char test3[9] = "foo3.txt";
		create(test3);
	}*/

	/*FILE *fp = fopen(argv[2], "r");
	char line[1000];
	fgets(line, 300, file);
	while(!feof(fp)){
		char*splitter[50];
		while(line[i] != '\0'){
			
		}
	}*/
	
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
	while(getline(in, str)){
		int i = 0;
		stringstream ssin(str);
		while(ssin.good() && i < 5){
			ssin >> array[i];
			i++;
		}
		if(array[0] == "CREATE"){
			char test[33];
			strcpy(test, array[1].c_str());
			create(test);
		}
		else if(array[0] == "DELETE"){
			char test[33];
			strcpy(test, array[1].c_str());
			delete_file(test);
		}
		else if(array[0] == "LIST"){
			list();
		}
		else if(array[0] == "IMPORT"){
			char test[33];
			strcpy(test, array[1].c_str());
			char test1[33];
			strcpy(test1, array[2].c_str());
			import(test, test1);
		}
	}
	write_inode_map();
	write_fbl();
	return 0;	 	
}
