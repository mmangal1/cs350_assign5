/*DELETE <SSFS file name>
Remove	the	file	named	<SSFS file name> from	SSFS,	and free	all	of	its	blocks,	including	the	inode,	all	data	
blocks, and	all	indirect	blocks associated	with	the	file
*/
void del(string ssfs_file_name){
	int inode_index = -1;
	/* find the inode for the given file name*/
	for(int i = 0; i < 256; i++){
		if(inode_mem[i] -> file_name == ssfs_file_name){
			cout << "found one: " << i << endl;
			inode_index = i;
			break; 
		}
	}
	if(inode_index == -1){
		fprintf(stderr, "the file you wish to delete does not exist on disk");
		exit(1);
	}
	/* look at file size blocks and free them*/
	//int double_count;
	int double_block;
	int freeing_block;
	int indirect_block_index = 0;
	int double_indirect_block_index = 0;
/*delete blocks associated with the inode*/
	//for(int i = 0; i < inode_map[inode_index].file_size; i++){ changed this line
	for(int i = 0; i < inode_map[inode_index].total_blocks; i++){
		if(i < 12){
			free_block_list[inode_map[inode_index].direct_ptrs[i] = 0;
		}else if(i < (block_size / 4) + 13){/* add 13 to account for the free block that is used to hold the indirect block*/
			if(inode_map[inode_index].indirect_ptrs != -1){
				fseek(fp, inode_map[inode_index].indirect_ptrs + (indirect_block_index * sizeof(int)), SEEK_SET);
				fread(&(freeing_block), sizeof(freeing_block), 1, fp);
				free_block_list[freeing_block] = 0;
				indirect_block_index++; /* increment the position in the indirect block*/
			}else break;
			if(i == (block_size / 4) + 12){
				free_block_list[inode_map[inode_index].indirect_ptrs = 0; /* free the indirect_ptrs block*/
				indirect_block_index = 0;
			}
		}else{
			if(inode_map[inode_index].dindirect_ptrs != -1){
				fseek(fp, inode_map[inode_index].dindirect_ptrs + (indirect_block_index * sizeof(int)), SEEK_SET);/*move to index in dindirect block*/
				fread(&(double_block), sizeof(double_block), 1, fp);/*read indirect block ptr*/
				fseek(fp, double_block + (double_indirect_block_index * sizeof(int)), SEEK_SET);/* move index in indirect block*/
				fread(&(freeing_block), sizeof(freeing_block), 1, fp);/* read block number to free in the direct block*/
				free_block_list[freeing_block] = 0;	/*free block*/
				double_indirect_block_index++; /*move to next index in indirect block*/
				if(double_indirect_block_index == block_size /4){ /*if we have read an entire indirect block*/
					double_indirect_block_index = 0; /*prepare to read first int in the next indirect block*/
					indirect_block_index++; /*move to next index in double indirect block*/
					free_block_list[double_block] = 0; /* free indirect block that we have completely read*/
				}
			}else break;
			//if(i == (block_size / 4) + 11) free_block_list[inode_map[inode_index].indirect_ptrs = 0; /* free the indirect_ptrs block*/
		}
	}
	if(inode_map[inode_index].dindirect_ptrs != -1)
		free_block_list[inode_map[inode_index].dindirect_ptrs = 0; /* free the dindirect_ptrs block*/
/* all blocks should be freed*/
	//inode_map[inode_index].file_name = NULL; /*remove the file name from the node*/
	inode_map[inode_index] = 0;
	delete inode_mem[inode_index];
	inode_mem[inode_index] = NULL;

}


/*
IMPORT <SSFS file name> <unix file name>
This command causes the contents of <SSFS file name>, if any,  to be overwritten by the	contents of the	linux	
file (in the CS	file system) named <unix file name>. If	a file	named	<SSFS file name> does not yet exist in	
SSFS, then a new one should be created	to contain the	contents of <unix file name>.	
*/
void import(string ssfs_file_name, string unix_file_name){
	int inode_index = find_inode_index(ssfs_file_name);

	if(inode_index != -1){
		del(string ssfs_file_name);
	}
	create(string ssfs_file_name);/* this will initialize a node to hold the contents of the file*/
	inode_index = find_inode_index(ssfs_file_name);


	struct stat st;	
	if(stat(unix_filename.c_str(), &st) != 0){
		fprintf(stderr, "file_size is 0");
		exit(1);
	}
	int file_size = st.st_size;
	inode_mem[inode_index] -> size = file_size;
	inode_mem[inode_index] -> total_blocks = ;
	inode_mem[inode_index] -> index = inode_index;



	//number of blocks needed to store the file
	int num_blocks;
	if(file_size < block_size){
		num_blocks = 1;
	}else if (file_size % block_size == 0){
		num_blocks = (file_size / block_size);
	}else{
		num_blocks = (file_size / block_size) + 1;
	}
	inode_mem[inode_index] -> total_blocks = num_blocks;
	//vector to hold free blocks we will be giving to the inode
	vector<int> freeBlocks = read_fbl();

	//figure out some algorithm to determine how many additional blocks are needed for the indirect ptrs
	if(num_blocks > 12 && num_blocks <= 12 + (block_size / 4)){
		num_blocks++;  /* need an extra block for the indirect block*/
	}else if(num_blocks > 12 + (block_size / 4){/*need double indirect ptrs*/
		int temp = num_blocks;
		temp = temp - (12 + (block_size / 4));
		while(temp > 0){
			num_blocks++;
			temp = temp -(block_size / 4);
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
	FILE* fp = fopen(disk_name.c_str(), "wb");

	//number if ints an indirect block can hold
	int num_indirect = block_size / 4;
	cout << "num_indirect = " << num_indirect << endl;
	int count = 0; // determine if we need to move to the next indirect block
	int indirect_block;
	int next = 0;
	for(int j = 0; j < num_blocks; j++){
		if(j < 12){
			inode_import->direct_ptrs[j] = freeBlocks.at(j);
		}else if(j >= 12 && j < num_indirect + 13){
			// write the next free block to the indirect block
			if(j == 12){
				inode_import->indirect_ptrs = freeBlocks.at(j);
				// will move to the indirect block to begin writing
				fseek(fp, (inode_import->indirect_ptrs * block_size), SEEK_SET);
			}else{
	//TODO			//write free blocks into the ptr's block
				// pass to the scheduler thread to write to the disk file
				fwrite(&freeBlocks.at(j), sizeof(int), 1, fp);
			}
			
		}else{
			if(j == num_indirect + 13){
				cout << "we got here" << endl;
				inode_import->dindirect_ptrs = freeBlocks.at(j);
				// will move to the indirect block to begin writing
				fseek(fp, (inode_import->dindirect_ptrs * block_size), SEEK_SET);
			}else{
		//TODO		//write free blocks into the ptr's block
				// pass to the scheduler thread to write to the disk file
				//have to move to next indirect block
				if(count == 0/*count % num_indirect == 0*/){
				//cout << " in here???" << endl;
					fseek(fp, (inode_import->dindirect_ptrs * block_size) + (block_size * next), SEEK_SET);
					fwrite(&freeBlocks.at(j), sizeof(int), 1, fp);
					indirect_block = freeBlocks.at(j);
					fseek(fp, (freeBlocks.at(j) * block_size), SEEK_SET);
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
	write_file_to_disk_using_inode_free_blocks(unix_file_name)


	
	fclose(fp);//close the disk file
}





//HELPER FUNCTION TO WRITE THE CONTENTS OF A FILE TO THE DISK BASED ON THE BLOCK PTRS IN ITS INODE
void write_file_to_disk_using_inode_free_blocks(string unix_file_name){
	int inode_index = -1;
	/* find the inode for the given file name*/
	for(int i = 0; i < 256; i++){
		if(inode_mem[i] -> file_name == ssfs_file_name){
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
	FILE* fp = fopen(disk_name.c_str(), "wb");
	FILE* unix_fp = fopen(unix_file_name.c_str(), "rb");
	char buff[sb.block_size];
	//for(int i = 0; i < inode_map[inode_index].file_size; i++){ changed this line
	for(int i = 0; i < inode_map[inode_index].total_blocks; i++){
		if(i < 12){
			fseek(fp, inode_map[inode_index].direct_ptrs[i], SEEK_SET);
			fread(&buff, sb.block_size, 1, unix_fp);
			fwrite(&buff, sb.block_size, 1, fp);
		}else if(i < (block_size / 4) + 13){/* add 13 to account for the free block that is used to hold the indirect block*/
			if(inode_map[inode_index].indirect_ptrs != -1){
				fseek(fp, inode_map[inode_index].indirect_ptrs + (indirect_block_index * sizeof(int)), SEEK_SET);
				fread(&(freeing_block), sizeof(freeing_block), 1, fp);
				fseek(fp, freeing_block, SEEK_SET);
				fread(&buff, sb.block_size, 1, unix_fp);
				fwrite(&buff, sb.block_size, 1, fp);
				indirect_block_index++; /* increment the position in the indirect block*/
			}else break;
			if(i == (block_size / 4) + 12){
				indirect_block_index = 0;
			}
		}else{
			if(inode_map[inode_index].dindirect_ptrs != -1){
				fseek(fp, inode_map[inode_index].dindirect_ptrs + (indirect_block_index * sizeof(int)), SEEK_SET);/*move to index in dindirect block*/
				fread(&(double_block), sizeof(double_block), 1, fp);/*read indirect block ptr*/
				fseek(fp, double_block + (double_indirect_block_index * sizeof(int)), SEEK_SET);/* move index in indirect block*/
				fread(&(freeing_block), sizeof(freeing_block), 1, fp);/* read block number to free in the direct block*/
				fseek(fp, freeing_block, SEEK_SET);
				fread(&buff, sb.block_size, 1, unix_fp);
				fwrite(&buff, sb.block_size, 1, fp);
				double_indirect_block_index++; /*move to next index in indirect block*/
				if(double_indirect_block_index == block_size /4){ /*if we have read an entire indirect block*/
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
int find_inode_index(string ssfs_file_name){
	int inode_index = -1;
	/* find the inode for the given file name*/
	for(int i = 0; i < 256; i++){
		if(inode_mem[i] -> file_name == ssfs_file_name){
			cout << "found one: " << i << endl;
			inode_index = i;
			break; 
		}
	}
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

