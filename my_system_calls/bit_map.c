#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/buffer_head.h> // buffer_head
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/blkdev.h>

#include "../module_parameters.h"
#include "new_system_call.h"

static int path_blocks_e_f[SIZE_BIT_MAP];

char* strremove(char* str, const char* sub) {

    size_t len = strlen(sub);

    if (len > 0) {
        char* p = str;
        while ((p = strstr(p, sub)) != NULL) {
            memmove(p, p + len, strlen(p + len) + 1);
        }
    }
    return str;
}

int populate_bit_map(int index_block_bit_map) {

    struct block_device* bd_loopx;
    struct buffer_head* bh;
    int offset = 0;
    int i;

    printk(" ****************** populate_bit_map ****************** START \n");

    bd_loopx = blkdev_get_by_path(MY_DEV_LOOPX, FMODE_WRITE, NULL); // file is open for writing
    if (!bd_loopx) {
        printk(KERN_ERR " ---- ERROR in blkdev_get_by_path \n");
        return 1;
    }
    printk(KERN_INFO " ---- blkdev_get_by_path .... ok \n");

    bh = __bread(bd_loopx, index_block_bit_map, MY_BLOCK_SIZE);
    if (!bh) {
        printk(KERN_ERR " ---- ERROR in __bread \n");
        return 1;
    }
    printk(KERN_INFO " ---- __bread .... ok \n");

    // populate the bit map with 0
    for (i = 0; i < SIZE_BIT_MAP; i++) {
        memcpy(bh->b_data + offset, BIT_CLEAR, sizeof(char));
        offset += sizeof(char);
    }
    printk(KERN_INFO " ---- Size of bit map = %d \n", SIZE_BIT_MAP);
    printk(KERN_INFO " ---- Block '%d' contents: \n", index_block_bit_map);
    printk(KERN_INFO "%s", bh->b_data);
    printk(KERN_INFO " ---- \n");

    mark_buffer_dirty(bh);             // mark the buffer as dirty
    brelse(bh);                        // releases the buffer head
    blkdev_put(bd_loopx, FMODE_WRITE); // releases the block device - mode WRITE
    printk(KERN_INFO " ---- release .... ok \n");

    // set the bit in the block bit map    
    set_bit_in_bit_map(index_block_bit_map, BIT_SET);

    printk(" ****************** populate_bit_map ****************** END \n");
    return 0;
}

int set_bit_in_bit_map(int block_to_set, char* mode_bit) {

    struct block_device* bd_loopx;
    struct buffer_head* bh;
    int offset = sizeof(char) * block_to_set;

    printk(" ****************** set_bit_in_bit_map ****************** START \n");

    bd_loopx = blkdev_get_by_path(MY_DEV_LOOPX, FMODE_WRITE, NULL); // file is open for writing
    if (!bd_loopx) {
        printk(KERN_ERR " ---- ERROR in blkdev_get_by_path \n");
        return 1;
    }
    printk(KERN_INFO " ---- blkdev_get_by_path .... ok \n");

    bh = __bread(bd_loopx, BLOCK_NUMBER_BIT_MAP, MY_BLOCK_SIZE);
    if (!bh) {
        printk(KERN_ERR " ---- ERROR in __bread \n");
        return 1;
    }
    printk(KERN_INFO " ---- __bread .... ok \n");

    if (!strcmp(mode_bit, BIT_CLEAR)) {
        printk(KERN_INFO " ---- Remove block in the position number = %d \n", block_to_set);
    }
    else if (!strcmp(mode_bit, BIT_SET)) {
        printk(KERN_INFO " ---- New block set in the position number = %d \n", block_to_set);
    }
    printk(KERN_INFO " ---- Block '%d' contents: \n", BLOCK_NUMBER_BIT_MAP);
    printk(KERN_INFO "%s ---> ", bh->b_data); // old data block
    memcpy(bh->b_data + offset, mode_bit, sizeof(char)); // set bit in the block bit map     
    printk(KERN_INFO "%s", bh->b_data);
    printk(KERN_INFO " ---- \n");

    mark_buffer_dirty(bh);             // mark the buffer as dirty
    brelse(bh);                        // releases the buffer head
    blkdev_put(bd_loopx, FMODE_WRITE); // releases the block device - mode WRITE
    printk(KERN_INFO " ---- release .... ok \n");

    printk(" ****************** set_bit_in_bit_map ****************** END \n");
    return 0;
}

int add_index_block_to_read(char* new_block_to_read) {

    struct block_device* bd_loopx;
    struct buffer_head* bh;

    printk(" ****************** add_index_block_to_read ****************** START \n");

    bd_loopx = blkdev_get_by_path(MY_DEV_LOOPX, FMODE_WRITE, NULL); // file is open for writing
    if (!bd_loopx) {
        printk(KERN_ERR " ---- ERROR in blkdev_get_by_path \n");
        return 1;
    }
    printk(KERN_INFO " ---- blkdev_get_by_path .... ok \n");

    bh = __bread(bd_loopx, BLOCK_NUMBER_BIT_MAP, MY_BLOCK_SIZE);
    if (!bh) {
        printk(KERN_ERR " ---- ERROR in __bread \n");
        return 1;
    }
    printk(KERN_INFO " ---- __bread .... ok \n");

    printk(KERN_INFO " ---- Add block index = %s \n", new_block_to_read);
    printk(KERN_INFO " ---- Block '%d' contents: \n", BLOCK_NUMBER_BIT_MAP);
    printk(KERN_INFO "%s ---> ", bh->b_data); // old data block
    strcat(bh->b_data, new_block_to_read); // add the new index for the order of reading blocks
    printk(KERN_INFO "%s", bh->b_data);
    printk(KERN_INFO " ---- \n");

    mark_buffer_dirty(bh);             // mark the buffer as dirty
    brelse(bh);                        // releases the buffer head
    blkdev_put(bd_loopx, FMODE_WRITE); // releases the block device - mode WRITE
    printk(KERN_INFO " ---- release .... ok \n");

    printk(" ****************** add_index_block_to_read ****************** END \n");
    return 0;
}

int sub_index_block_to_read(char* block_to_invalidate) {

    struct block_device* bd_loopx;
    struct buffer_head* bh;

    printk(" ****************** sub_index_block_to_read ****************** START \n");

    bd_loopx = blkdev_get_by_path(MY_DEV_LOOPX, FMODE_WRITE, NULL); // file is open for writing
    if (!bd_loopx) {
        printk(KERN_ERR " ---- ERROR in blkdev_get_by_path \n");
        return 1;
    }
    printk(KERN_INFO " ---- blkdev_get_by_path .... ok \n");

    bh = __bread(bd_loopx, BLOCK_NUMBER_BIT_MAP, MY_BLOCK_SIZE);
    if (!bh) {
        printk(KERN_ERR " ---- ERROR in __bread \n");
        return 1;
    }
    printk(KERN_INFO " ---- __bread .... ok \n");

    printk(KERN_INFO " ---- Sub block index = %s \n", block_to_invalidate);
    printk(KERN_INFO " ---- Block '%d' contents: \n", BLOCK_NUMBER_BIT_MAP);
    printk(KERN_INFO "%s ---> ", bh->b_data); // old data block
    bh->b_data = strremove(bh->b_data, block_to_invalidate);  // delete the index from the order of reading blocks
    printk(KERN_INFO "%s", bh->b_data);
    printk(KERN_INFO " ---- \n");

    mark_buffer_dirty(bh);             // mark the buffer as dirty
    brelse(bh);                        // releases the buffer head
    blkdev_put(bd_loopx, FMODE_WRITE); // releases the block device - mode WRITE
    printk(KERN_INFO " ---- release .... ok \n");

    printk(" ****************** sub_index_block_to_read ****************** END \n");
    return 0;
}

int get_path_blocks_empty_or_full(char* mode_blocks, int path_blocks_e_f[]) {

    struct block_device* bd_loopx;
    struct buffer_head* bh;
    int i;
    int number_blocks_with_mode = 0;
    int offset = 0;

    printk(" ****************** get_path_blocks_empty_or_full ****************** START \n");

    bd_loopx = blkdev_get_by_path(MY_DEV_LOOPX, FMODE_READ, NULL); // file is open for reading
    if (!bd_loopx) {
        printk(KERN_ERR " ---- ERROR in blkdev_get_by_path \n");
        return 1;
    }
    printk(KERN_INFO " ---- blkdev_get_by_path .... ok \n");

    bh = __bread(bd_loopx, BLOCK_NUMBER_BIT_MAP, MY_BLOCK_SIZE);
    if (!bh) {
        printk(KERN_ERR " ---- ERROR in __bread \n");
        return 1;
    }
    printk(KERN_INFO " ---- __bread .... ok \n");

    // read bit in bit map and create a path to blocks full
    for (i = 0; i < SIZE_BIT_MAP; i++) {

        if (bh->b_data[offset] == *mode_blocks) {

            path_blocks_e_f[number_blocks_with_mode] = i;
            number_blocks_with_mode++;
            printk(KERN_INFO "bh->b_data[%d] = %c \n", offset, bh->b_data[offset]);
        }
        offset++;
    }

    // if there is free space in the block device
    if (!strcmp(mode_blocks, BIT_CLEAR)) {

        printk(KERN_INFO " ---- List empty blocks ");
        printk(KERN_INFO " ---- SIZE_BIT_MAP .......... = %d \n", SIZE_BIT_MAP);
        printk(KERN_INFO " ---- number_blocks_clear = %d \n", number_blocks_with_mode);

        for (i = 0; i < number_blocks_with_mode; i++) {
            printk(KERN_INFO "path_blocks_empty[%d] = %d \n", i, path_blocks_e_f[i]);
        }
    }
    else if (!strcmp(mode_blocks, BIT_SET)) {

        printk(KERN_INFO " ---- List full blocks");
        printk(KERN_INFO " ---- SIZE_BIT_MAP .......... = %d \n", SIZE_BIT_MAP);
        printk(KERN_INFO " ---- number_blocks_set = %d \n", number_blocks_with_mode);

        for (i = 0; i < number_blocks_with_mode; i++) {
            printk(KERN_INFO "path_blocks_full[%d] = %d \n", i, path_blocks_e_f[i]);
        }
    }
    printk(KERN_INFO " ---- Block '%d' contents: \n", BLOCK_NUMBER_BIT_MAP);
    printk(KERN_INFO "%s", bh->b_data);
    printk(KERN_INFO " ---- \n");

    brelse(bh);                       // releases the buffer head
    blkdev_put(bd_loopx, FMODE_READ); // releases the block device - mode READ
    printk(KERN_INFO " ---- release .... ok \n");

    printk(" ****************** get_path_blocks_empty_or_full ****************** END \n");
    return number_blocks_with_mode;
}

int check_if_exist_writing_block(int index_block_to_check) {

    int i;
    int number_blocks_written = 0;
    int return_value;
    int block_is_found = 0;

    printk(" ****************** check_if_exist_writing_block ****************** START \n");

    number_blocks_written = get_path_blocks_empty_or_full(BIT_SET, path_blocks_e_f);

    for (i = 0; i < number_blocks_written; i++) {
        if (index_block_to_check == path_blocks_e_f[i]) {
            printk(KERN_INFO " ---- FULL block selected: [%d] = path_blocks_e_f[%d] = %d \n", index_block_to_check, i, path_blocks_e_f[i]);
            return_value = FULL_BLOCK;
            block_is_found = 1;
            break;
        }
    }

    if (block_is_found == 0) {
        printk(KERN_INFO " ---- EMPTY block selected: [%d] \n", index_block_to_check);
        return_value = EMPTY_BLOCK;
    }

    printk(" ****************** check_if_exist_writing_block ****************** END \n");
    return return_value;
}

