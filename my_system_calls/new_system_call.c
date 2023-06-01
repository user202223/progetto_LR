#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/buffer_head.h> // buffer_head
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>

#include <linux/types.h>

#include "../module_parameters.h"
#include "new_system_call.h"

/*
 ---- system_call_number_index ----

    #define FIRST_NI_SYSCALL	134     ---> put_data
    #define SECOND_NI_SYSCALL	174     ---> get_data
    #define THIRD_NI_SYSCALL	182     ---> invalidate_data
    #define FOURTH_NI_SYSCALL	183
    #define FIFTH_NI_SYSCALL	214
    #define SIXTH_NI_SYSCALL	215
    #define SEVENTH_NI_SYSCALL	236

 ---- system calls ----

    syscall(system_call_number_index, parameter_1, parameter_2, ... );

    134 ----> PUT_DATA -----------> put_data(char* source, size_t size);
    174 ----> GET_DATA -----------> get_data(int offset, char* destination, size_t size)
    182 ----> INVALIDATE_DATA ----> invalidate_data(int offset)

 ---- __brelse() - brelse() ----

    The brelse() function checks if the buffer_head structure is marked as dirty and writes the buffer to disk
    if necessary before calling __brelse() to release the buffer_head structure.
    On the other hand, __brelse() simply releases the buffer_head structure without performing any additional checks.
    If the buffer_head structure needs to be written to disk before being released, brelse() is used. Otherwise, __brelse() is used for a faster release.

 ---- __bread ----

    static inline struct buffer_head *__bread(struct block_device *bdev, sector_t block, unsigned size)

    struct block_device *bdev: A pointer to the block device that contains the block to be read.
    sector_t block:            The block number to be read.
    unsigned size:             The size of the block to be read.

*/

DEFINE_MUTEX(my_mutex);
static int path_blocks_e_f[SIZE_BIT_MAP];

int check_if_file_exist(void) {

    struct file* file_test = filp_open(PATH_UNIQUE_FILE_NAME, O_RDONLY, 0);

    if (file_test == NULL || IS_ERR(file_test)) {
        printk(KERN_INFO "[check_if_file_exist] the file does not exist or is not accesible: %s \n", PATH_UNIQUE_FILE_NAME);
        return 1;  // ENODEV
    }
    else {
        filp_close(file_test, NULL);
        printk(KERN_INFO "[check_if_file_exist] the file exist: %s \n", PATH_UNIQUE_FILE_NAME);
        return 0;
    }
}

/**
 * @param source user message from which take 'size' bytes to write
 * @param size   the number of bytes to write from the user message
 * @result       the index of the block where data is been written
*/
int put_data(char* source, size_t size) {

    struct block_device* bd_loopx;
    struct buffer_head* bh;

    int block_for_writing = 0; // the specific block I want to write    
    char index_str[SIZE_INDEX_STR];
    int number_blocks_free = 0;

    mutex_lock(&my_mutex);

    printk(KERN_INFO " ======================= PUT_DATA ======================= START \n");

    if (check_if_file_exist() == 0) {

        printk(KERN_INFO " ---- user message .......... %s \n", source);
        printk(KERN_INFO " ---- printing only %ld bytes: %.*s\n", size, (int)size, source);

        // if there is not free space in the block device   
        number_blocks_free = get_path_blocks_empty_or_full(BIT_CLEAR, path_blocks_e_f);

        if (number_blocks_free == 0) {
            printk(KERN_INFO " ---- there is not space in the block device, number_block_free = %d \n", number_blocks_free);
            block_for_writing = ENOMEM;
            printk(KERN_INFO " ---- block_for_writing = %d \n", block_for_writing);
        }
        else {
            printk(KERN_INFO " ---- there is space in the block device, number_block_free = %d \n", number_blocks_free);

            // select first free block with bit=0 in the bit map
            block_for_writing = path_blocks_e_f[0];

            // mark the free block as a writing block
            sprintf(index_str, "[%d]", path_blocks_e_f[0]);
            set_bit_in_bit_map(path_blocks_e_f[0], BIT_SET);
            add_index_block_to_read(index_str);

            bd_loopx = blkdev_get_by_path(MY_DEV_LOOPX, FMODE_WRITE, NULL); // file is open for writing
            if (!bd_loopx) {
                printk(KERN_ERR " ---- ERROR in blkdev_get_by_path \n");
                return 1;
            }
            printk(KERN_INFO " ---- blkdev_get_by_path .... ok \n");

            bh = __bread(bd_loopx, block_for_writing, MY_BLOCK_SIZE);
            if (!bh) {
                printk(KERN_ERR " ---- ERROR in __bread \n");
                return 1;
            }
            printk(KERN_INFO " ---- __bread .... ok \n");

            // write user message in the free block selected
            memcpy(bh->b_data, &size, 2);
            memcpy(bh->b_data + 2, source, size);

            printk(KERN_INFO " ---- Block %d content after writing: \n", block_for_writing);
            printk(KERN_INFO " ---- SIZE \n");
            printk(KERN_INFO "%d", bh->b_data[0]);
            printk(KERN_INFO "%d", bh->b_data[1]);

            printk(KERN_INFO " ---- MESSAGE \n");
            printk(KERN_INFO "%s", bh->b_data + 2);
            printk(KERN_INFO " ---- writing .... ok \n");

            mark_buffer_dirty(bh);             // Mark the buffer as dirty
            brelse(bh);                        // releases the buffer head
            blkdev_put(bd_loopx, FMODE_WRITE); // releases the block device - mode WRITE
            printk(KERN_INFO " ---- release .... ok \n");
        }
    }
    else {
        block_for_writing = ENODEV;
    }
    printk(" ======================= PUT_DATA ======================= END \n");

    mutex_unlock(&my_mutex);

    return block_for_writing;
}

/**
 * @param offset      index of the block to read
 * @param destination area where save the data read from the block selected
 * @param size        the number of bytes to read from the block selected
 * @result            the number of bytes read from the block selected
*/
int get_data(int offset, char* destination, size_t size) {

    struct block_device* bd_loop0;
    struct buffer_head* bh;

    int i;
    int number_bytes_read = 0;
    int return_value;

    mutex_lock(&my_mutex);

    printk(" ======================= GET_DATA ======================= START \n");

    if (check_if_file_exist() == 0) {

        // check if the index block is currently a full block
        if (check_if_exist_writing_block(offset) == EMPTY_BLOCK) {
            printk(KERN_INFO " ---- the selected block is empty and not suitable for reading: block_number = %d \n", offset);
            return_value = ENODATA;
            printk(KERN_INFO " ---- return_value = %d \n", return_value);
        }
        else {
            printk(KERN_INFO " ---- the selected block is full and suitable for reading: block_number = %d \n", offset);

            bd_loop0 = blkdev_get_by_path(MY_DEV_LOOPX, FMODE_READ, NULL); // file is open for reading
            if (!bd_loop0) {
                printk(KERN_ERR " ---- ERROR in blkdev_get_by_path \n");
                return 1;
            }
            printk(KERN_INFO " ---- blkdev_get_by_path .... ok \n");

            bh = __bread(bd_loop0, offset, MY_BLOCK_SIZE);
            if (!bh) {
                printk(KERN_ERR " ---- ERROR in __bread \n");
                return 1;
            }
            printk(KERN_INFO " ---- __bread .... ok \n");

            // print read data
            printk(KERN_INFO " ---- Block %d content for reading: \n", offset);
            printk(KERN_INFO " ---- SIZE \n");
            printk(KERN_INFO "%d", bh->b_data[0]);
            printk(KERN_INFO "%d", bh->b_data[1]);

            printk(KERN_INFO " ---- MESSAGE \n");
            printk(KERN_INFO "%s", bh->b_data + 2);
            printk(KERN_INFO " ---- reading .... ok \n");

            printk(KERN_INFO " ---- reading only %ld bytes: %.*s\n", size, (int)size, bh->b_data + 2);

            // data written is null
            if (bh->b_data == NULL) {
                printk(KERN_INFO " ---- data written is null \n");
                number_bytes_read = 0;
            }
            else {
                // save 'size' bytes read from the block selected in 'destination'
                for (i = 0; i < size; i++) {
                    destination[i] = bh->b_data[i + 2];
                    number_bytes_read++;
                    printk(KERN_INFO " ---- destination[i] = %c ---- bh->b_data[i] = %c \n", destination[i], bh->b_data[i + 2]);
                }
                destination[i] = '\0';
                printk(KERN_INFO " ---- message_to_read = %s \n", destination);
                printk(KERN_INFO " ---- reading 'size' bytes .... ok \n");
            }
            return_value = number_bytes_read;

            brelse(bh);                       // releases the buffer head
            blkdev_put(bd_loop0, FMODE_READ); // releases the block device - mode READ
            printk(KERN_INFO " ---- release .... ok \n");
        }
    }
    else {
        return_value = ENODEV;
    }
    printk(" ======================= GET_DATA ======================= END \n");

    mutex_unlock(&my_mutex);

    return return_value;
}

/**
 * @param offset index of the block to invalidate
 * @result       ENODATA error if there is not corret index associated to a block
*/
int invalidate_data(int offset) {

    struct block_device* bd_loopx;
    struct buffer_head* bh;

    char index_str[SIZE_INDEX_STR];
    int return_value = 0;

    mutex_lock(&my_mutex);

    printk(" ======================= INVALIDATE_DATA ======================= START \n");

    if (check_if_file_exist() == 0) {

        // check if the index block is currently a full block    
        if (check_if_exist_writing_block(offset) == EMPTY_BLOCK) {
            printk(" ---- the selected block is empty and cannot be invalidated: block_number = %d \n", offset);
            return_value = ENODATA;
            printk(KERN_INFO " ---- return_value = %d \n", return_value);
        }
        else {
            printk(" ---- the selected block is full and can be invalidated: block_number = %d \n", offset);

            // remove the index in the bit map
            sprintf(index_str, "[%d]", offset);
            set_bit_in_bit_map(offset, BIT_CLEAR);
            sub_index_block_to_read(index_str);

            bd_loopx = blkdev_get_by_path(MY_DEV_LOOPX, FMODE_WRITE, NULL); // file is open for writing
            if (!bd_loopx) {
                printk(KERN_ERR " ---- ERROR in blkdev_get_by_path \n");
                return 1;
            }
            printk(KERN_INFO " ---- blkdev_get_by_path .... ok \n");

            bh = __bread(bd_loopx, offset, MY_BLOCK_SIZE);
            if (!bh) {
                printk(KERN_ERR " ---- ERROR in __bread \n");
                return 1;
            }
            printk(KERN_INFO " ---- __bread .... ok \n");

            // clear block content
            printk(KERN_INFO " ---- Block %d contents: \n", offset);

            printk(KERN_INFO " ---- SIZE \n");
            printk(KERN_INFO "%d", bh->b_data[0]);
            printk(KERN_INFO "%d", bh->b_data[1]);

            printk(KERN_INFO " ---- MESSAGE \n");
            printk(KERN_INFO "%s", bh->b_data + 2);
            printk(KERN_INFO " ---- reading original .... ok \n");

            memcpy(bh->b_data, "", 1);
            printk(KERN_INFO "%s \n", bh->b_data);
            printk(KERN_INFO " ---- reading now empty .... ok \n");

            mark_buffer_dirty(bh);             // Mark the buffer as dirty
            brelse(bh);                        // releases the buffer head
            blkdev_put(bd_loopx, FMODE_WRITE); // releases the block device - mode WRITE
            printk(KERN_INFO " ---- release .... ok \n");
        }
    }
    else {
        return_value = ENODEV;
    }
    printk(" ======================= INVALIDATE_DATA ======================= END \n");

    mutex_unlock(&my_mutex);

    return return_value;
}