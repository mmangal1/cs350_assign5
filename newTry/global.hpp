#ifndef GLOBAL_HPP
#define GLOBAL_HPP
#include <stdio.h>
#include "super_block.hpp"
#include <vector>
#include <string>
#include <queue>
#include <pthread.h>
#include "inode.hpp"
#include "shared.hpp"

using namespace std;
extern pthread_mutex_t mutex1;
extern pthread_mutex_t mutex2;
extern pthread_cond_t full, empty;
extern int in;
extern int out;
extern queue<shared*> buffer;
extern superblock sb;
extern int inode_map[256];
extern vector<inode*> inode_mem; //inodes on disk
extern int* free_block_list;
extern int fbl_block_count;
extern int read_free_mem_imap();
extern vector<int> read_fbl();
extern void update_inode_map(int index, int bit_to_set);
extern void update_free_list(int index, int bit_to_set);
extern string disk_name;
extern void write_fbl();
extern void write_inode_map();
extern void write_inode_to_disk(inode *node, int index);
extern void add_to_buffer(shared *myShared);
extern shared* set_shared_struct(int operation, int block_num, char *data);

/* COMMANDS */
extern void create(char ssfs_file_name[]);
extern void import(char ssfs_file_name[], char unix_file_name[]);
extern void cat(char ssfs_file_name[]);
extern void delete_file(char ssfs_file_name[]);
extern void write(string ssfs_file_name, char c, int start_byte, int num_bytes);
extern void read(string ssfs_file_name, int start_byte, int num_bytes);
extern void list();
extern void shutdown();

/* HELPER FUNCTIONS */
void write_file_to_disk_using_inode_free_blocks(char ssfs_file_name[], char unix_file_name[]);
int find_inode_index(char ssfs_file_name[]);
#endif
