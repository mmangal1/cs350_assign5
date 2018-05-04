#include <iostream>
#include <string>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <cstring>
#include <pthread.h>
#include <queue>
#include "loader.hpp"
#include "global.hpp"

using namespace std;

void* scheduler(void * useless){
	while(true){
		pthread_mutex_lock(&mutex1);
		while(buffer.empty()){
			pthread_cond_wait(&full, &mutex1);
		}
		shared *obj = buffer.front();
		buffer.pop();
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&mutex1);
		if(obj -> operation == 0){
			FILE *fp = fopen(disk_name.c_str(), "rb");
			fseek(fp, obj -> block_num, SEEK_SET);
			if(obj -> readInt){
				fread(&(obj -> myInt), 4, 1, fp);
			}else{
				fread((obj -> data), obj -> size, 1, fp);
			}
			fclose(fp);
			obj -> done = true;
			pthread_cond_signal(&(obj ->condition));
		}
		else if(obj -> operation == 1){
			FILE *fp = fopen(disk_name.c_str(), "rb+");
			fseek(fp, obj -> block_num, SEEK_SET);
			//cout << "test: " << obj -> block_num << endl;
			if(obj -> readInt){
				cout << "test: " << obj -> myInt << endl; 
				fwrite(&(obj -> myInt), 4, 1, fp);
			}else{
				fwrite((obj -> data), obj -> size, 1, fp);
			}
			fclose(fp);
			obj -> done = true;
			pthread_cond_signal(&(obj -> condition));
			//cout << "write detected" << endl;
		}
		else if(obj -> operation == 2){
			cout << "shutdown detected" << endl;
			break;

		}
	}
	return NULL;
}

void* parser(void *file){
	ifstream in((char*)file);
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
		}else if(array[0] == "SHUTDOWN"){
			shared *myShared = new shared;
			myShared -> operation = 2;
			add_to_buffer(myShared);
		}
		else if(array[0] == "CAT"){
			char test[33];
			strcpy(test, array[1].c_str());
			cat(test);
		}
	}
	in.close();
	return NULL;
}

int main(int args, char** argv){
	
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

	if(args == 3){
		pthread_t scheduler_thread;
		pthread_create(&scheduler_thread, NULL, scheduler, (void*)argv[1]);
		pthread_t thread_one;
		pthread_create(&thread_one, NULL, parser, (void*)argv[2]);
		pthread_join(thread_one, NULL);	
		pthread_join(scheduler_thread, NULL);
	}
	else if(args == 4){
		cout << "here" << endl;
		pthread_t scheduler_thread;
		pthread_create(&scheduler_thread, NULL, scheduler, (void*)argv[1]);
		pthread_t thread_two;
		pthread_create(&thread_two, NULL, parser, (void*)argv[3]);	
		pthread_t thread_one;
		pthread_create(&thread_one, NULL, parser, (void*)argv[2]);

		pthread_join(thread_two, NULL);	
		pthread_join(thread_one, NULL);	
		pthread_join(scheduler_thread, NULL);

	}
	else if(args == 5){
		pthread_t scheduler_thread;
		pthread_create(&scheduler_thread, NULL, scheduler, (void*)argv[1]);
		pthread_t thread_three;
		pthread_create(&thread_three, NULL, parser, (void*)argv[4]);	
		pthread_t thread_two;
		pthread_create(&thread_two, NULL, parser, (void*)argv[3]);	
		pthread_t thread_one;
		pthread_create(&thread_one, NULL, parser, (void*)argv[2]);
		pthread_join(thread_three, NULL);
		pthread_join(thread_two, NULL);	
		pthread_join(thread_one, NULL);	
		pthread_join(scheduler_thread, NULL);

	}
	else if(args == 6){
		pthread_t scheduler_thread;
		pthread_create(&scheduler_thread, NULL, scheduler, (void*)argv[1]);
		pthread_t thread_four;
		pthread_create(&thread_four, NULL, parser, (void*)argv[5]);	
		pthread_t thread_three;
		pthread_create(&thread_three, NULL, parser, (void*)argv[4]);	
		pthread_t thread_two;
		pthread_create(&thread_two, NULL, parser, (void*)argv[3]);	
		pthread_t thread_one;
		pthread_create(&thread_one, NULL, parser, (void*)argv[2]);
		pthread_join(thread_four, NULL);
		pthread_join(thread_three, NULL);
		pthread_join(thread_two, NULL);	
		pthread_join(thread_one, NULL);	
		pthread_join(scheduler_thread, NULL);

	}
	/*while(true){
		cout << buffer.size() << endl;
		pthread_mutex_lock(&mutex1);
		while(buffer.empty()){
			pthread_cond_wait(&full, &mutex1);
		}
		shared *obj = buffer.front();
		buffer.pop();
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&mutex1);
		if(obj -> operation == 2){
			cout << "in op " << endl;
			break;

		}
	}*/


	write_inode_map();
	write_fbl();
	return 0;	
}
