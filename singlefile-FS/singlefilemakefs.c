#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../module_parameters.h"
#include "singlefilefs.h"

/*
	This makefs will write the following information onto the disk
	- BLOCK 0, superblock;
	- BLOCK 1, inode of the unique file (the inode for root is volatile);
*/

int main(int argc, char* argv[]) {

	int fd;
	ssize_t ret;
	struct onefilefs_sb_info sb;
	struct onefilefs_inode root_inode;
	struct onefilefs_inode file_inode;

	if (argc != 2) {
		printf("[SINGLE_FILE_MAKE_FS] Usage: ./singlefilemakefs $(FILE_IMAGE) \n");
		return -1;
	}

	fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		perror("[SINGLE_FILE_MAKE_FS] Error opening the device");
		return -1;
	}

	// pack the superblock
	sb.version = 1; // file system version
	sb.magic = MAGIC;
	sb.block_size = DEFAULT_BLOCK_SIZE;

	ret = write(fd, (char*)&sb, sizeof(sb));
	if (ret != DEFAULT_BLOCK_SIZE) {
		printf("[SINGLE_FILE_MAKE_FS] Bytes written [%d] are not equal to the default block size \n", (int)ret);
		printf("[SINGLE_FILE_MAKE_FS] DEFAULT_BLOCK_SIZE = %d \n", DEFAULT_BLOCK_SIZE);
		close(fd);
		return ret;
	}
	printf("[SINGLE_FILE_MAKE_FS] Super block written succesfully \n");

	// write file inode
	file_inode.mode = S_IFREG;
	file_inode.inode_no = SINGLEFILEFS_FILE_INODE_NUMBER;
	file_inode.file_size = SIZE_UNIQUE_FILE; // set the inode size

	printf("[SINGLE_FILE_MAKE_FS] File size is %ld \n", file_inode.file_size);

	ret = write(fd, (char*)&file_inode, sizeof(file_inode));
	if (ret != sizeof(root_inode)) {
		printf("[SINGLE_FILE_MAKE_FS] The file inode was not written properly \n");
		close(fd);
		return -1;
	}
	printf("[SINGLE_FILE_MAKE_FS] File inode written succesfully \n");

	close(fd);

	printf("[SINGLE_FILE_MAKE_FS] All file system metadata are written succesfully \n");

	return 0;
}
