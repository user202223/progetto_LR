#ifndef _ONEFILEFS_H
#define _ONEFILEFS_H

#include <linux/types.h>
#include <linux/fs.h>

// inode definition
struct onefilefs_inode {
	mode_t mode; // not exploited
	uint64_t inode_no;
	uint64_t data_block_number; // not exploited

	union {
		uint64_t file_size;
		uint64_t dir_children_count;
	};
};

// dir definition (how the dir datablock is organized)
struct onefilefs_dir_record {
	char filename[FILENAME_MAXLEN];
	uint64_t inode_no;
};

// superblock definition
struct onefilefs_sb_info {
	uint64_t version;
	uint64_t magic;
	uint64_t block_size;
	uint64_t inodes_count; // not exploited
	uint64_t free_blocks; // not exploited

	// padding to fit into a single block
	char padding[(4 * 1024) - (5 * sizeof(uint64_t))];
};

// file.c
extern const struct inode_operations onefilefs_inode_ops;
extern const struct file_operations onefilefs_file_operations;

// dir.c
extern const struct file_operations onefilefs_dir_operations;

#endif
