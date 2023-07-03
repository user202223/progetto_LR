#define NBLOCKS 10

// block_0 = super_block
// block_1 = i_node
// block_2 = bit_map
// block_3_4_5... = data contents
#define SIZE_UNIQUE_FILE (MY_BLOCK_SIZE * (NBLOCKS - 3))

extern struct mutex my_mutex;

// ==================== demo ====================
#define PATH_UNIQUE_FILE_NAME "./my_mount_point/" UNIQUE_FILE_NAME

// ==================== add_new_sc / my_system_calls ====================
#define MY_BLOCK_SIZE 4096
#define MY_DEV_LOOPX "/dev/loop7"

#define SIZE_BIT_MAP NBLOCKS    // the size of my bit map, that is how many blocks can exist
#define BLOCK_NUMBER_BIT_MAP 2  // the specific block where is the bit map

#define BIT_SET "1"             // set after put_data()
#define BIT_CLEAR "0"           // set after invalidate_data()

#define SIZE_INDEX_STR 10       // the size of all possible index in the block bit map: 0101[index][index]
#define FULL_BLOCK 0
#define EMPTY_BLOCK 1

// ====================  singlefile-FS ====================
#define MOD_NAME "[SINGLE_FILE_FS_SRC]"

#define MAGIC 0x42424242
#define DEFAULT_BLOCK_SIZE 4096
#define SB_BLOCK_NUMBER 0
#define DEFAULT_FILE_INODE_BLOCK 1

#define FILENAME_MAXLEN 255

#define SINGLEFILEFS_ROOT_INODE_NUMBER 10
#define SINGLEFILEFS_FILE_INODE_NUMBER 1

#define SINGLEFILEFS_INODES_BLOCK_NUMBER 1

#define UNIQUE_FILE_NAME "the-file"