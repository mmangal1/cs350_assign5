#ifndef SHARED_HPP
#define SHARED_HPP

typedef struct shared{
	int operation;
	int block_num;
	char *data;
	pthread_mutex_t mutex;
	pthread_cond_t condition;
	bool done;
	int size;
	bool readInt;
	int myInt;
} shared;

#endif
