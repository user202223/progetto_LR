#include <linux/types.h>

// ---- bit_map ----
char* strremove(char* str, const char* sub);
int populate_bit_map(int index_block_bit_map);
int set_bit_in_bit_map(int block_to_set, char* mode_bit);
int add_index_block_to_read(char* new_block_to_read);
int sub_index_block_to_read(char* block_to_invalidate);
int get_path_blocks_empty_or_full(char* mode_blocks, int path_blocks_e_f[]);
int check_if_exist_writing_block(int index_block_to_check);

// ---- new_system_call ----
int check_if_file_exist(void);
int put_data(char* source, size_t size);
int get_data(int offset, char* destination, size_t size);
int invalidate_data(int offset);