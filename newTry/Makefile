all: ssfs_mkdsk_driver main_driver

ssfs_mkdsk_driver: ssfs_mkdsk_driver.cpp ssfs_mkdsk.o
	g++ ssfs_mkdsk.o ssfs_mkdsk_driver.cpp -g -o ssfs_mkdsk

ssfs_mkdsk.o: ssfs_mkdsk.cpp ssfs_mkdsk.hpp
	g++ -g -c ssfs_mkdsk.cpp -o ssfs_mkdsk.o

main_driver: driver.o loader.o inode.o global.o
	g++ driver.o loader.o inode.o global.o -g -o ssfs

driver.o: driver.cpp global.hpp loader.hpp
	g++ -g -c driver.cpp

loader.o: loader.cpp loader.hpp global.hpp inode.hpp
	g++ -g -c loader.cpp

inode.o: inode.cpp global.hpp inode.hpp
	g++ -g -c inode.cpp

global.o: global.cpp global.hpp super_block.hpp
	g++ -g -c global.cpp 

clean:
	rm -f *.o
#	rm -f *.txt
	rm -f *.bin
	rm -f ssfs_mkdsk
	rm -f ssfs
