#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timekeeping.h>
#include <linux/time.h>
#include <linux/buffer_head.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/blkdev.h>

#include "../module_parameters.h"
#include "singlefilefs.h"

int get_path_to_read(int blocks_to_read[]) {

    struct block_device* bd_loopx;
    struct buffer_head* bh;
    int number_blocks_to_read = 0;
    char* path_bit_map;
    int num[SIZE_BIT_MAP];
    int i = 0;
    int j = 0;

    printk(" ****************** get_path_to_read ****************** START \n");

    bd_loopx = blkdev_get_by_path(MY_DEV_LOOPX, FMODE_READ, NULL); // file is open for reading
    if (!bd_loopx) {
        printk(KERN_ERR "[FILE-get_path_to_read] ERROR in blkdev_get_by_path \n");
        return 1;
    }
    printk(KERN_INFO "[FILE-get_path_to_read] blkdev_get_by_path .... ok \n");

    bh = __bread(bd_loopx, BLOCK_NUMBER_BIT_MAP, MY_BLOCK_SIZE);
    if (!bh) {
        printk(KERN_ERR "[FILE-get_path_to_read] ERROR in __bread \n");
        return 1;
    }
    printk(KERN_INFO "[FILE-get_path_to_read] __bread .... ok \n");

    printk(KERN_INFO "[FILE-get_path_to_read] Contents of block %d \n", BLOCK_NUMBER_BIT_MAP);
    printk(KERN_INFO "%s", bh->b_data);

    // block_0 = super_block
    // block_1 = i_node
    // block_2 = bit_map
    // block_3_4_5... = data contents
    for (i = 3; i < SIZE_BIT_MAP; i++) {

        if (bh->b_data[i] == *BIT_SET) {
            number_blocks_to_read++;
        }
    }
    printk(KERN_INFO "[FILE-get_path_to_read] number_blocks_to_read = %d \n", number_blocks_to_read);

    path_bit_map = bh->b_data;

    for (i = 0; i < number_blocks_to_read; i++) {
        num[i] = 0;
    }

    while (path_bit_map[i]) {
        if (path_bit_map[i] == '[') {
            i++;
            while (path_bit_map[i] >= '0' && path_bit_map[i] <= '9') {
                num[j] = num[j] * 10 + (path_bit_map[i] - '0');
                i++;
            }
            j++;
        }
        else {
            i++;
        }
    }

    for (i = 0; i < number_blocks_to_read; i++) {
        printk(KERN_INFO "[FILE-get_path_to_read] num[%d] = %d \n", i, num[i]);
    }

    for (i = 0; i < number_blocks_to_read; i++) {
        blocks_to_read[i] = num[i];
        printk(KERN_INFO "[FILE-get_path_to_read] blocks_to_read[%d] = %d \n", i, blocks_to_read[i]);
    }

    brelse(bh);                       // releases the buffer head
    blkdev_put(bd_loopx, FMODE_READ); // releases the block device - mode READ
    printk(KERN_INFO "[FILE-get_path_to_read] release .... ok \n");

    printk(" ****************** get_path_to_read ****************** END \n");
    return number_blocks_to_read;
}

extern struct mutex my_mutex;

/**
 * @param filp first argument struct file * is a pointer to a file object that represents the opened device file.
 * @param buf The second argument char __user * is a pointer to the user space buffer that will receive the data read from the device.
 * @param len The third argument size_t is the number of bytes to read from the device.
 * @param off The fourth argument loff_t * is a pointer to the current file position in the device file.
 *
 * @result The read function returns the number of bytes read from the device, or an error code if an error occurred.
*/
ssize_t onefilefs_read(struct file* filp, char __user* buf, size_t len, loff_t* off) {

    struct buffer_head* bh = NULL;
    struct inode* the_inode = filp->f_inode;
    uint64_t file_size_inode = the_inode->i_size;
    int ret;
    int block_to_read; // index of the block to be read from device
    char* data_user;
    int i;
    int number_blocks_to_read;
    int index_blocks_to_read[SIZE_BIT_MAP];
    int return_value_read;

    unsigned short value_size;
    unsigned short len_data_user = 0;

    mutex_lock(&my_mutex);

    printk(KERN_INFO " ******************************** onefilefs_read ******************************** START \n");

    printk(KERN_INFO "[FILE-onefilefs_read] file_size_inode = %lld \n", file_size_inode);

    // Allocate memory for the array
    data_user = kmalloc(SIZE_UNIQUE_FILE, GFP_KERNEL);
    if (!data_user) {
        printk(KERN_ALERT "[FILE-onefilefs_read] Failed to allocate memory\n");
        return -ENOMEM;
    }

    // check that *off is within boundaries
    if (*off >= file_size_inode) {
        return 0;
    }

    number_blocks_to_read = get_path_to_read(index_blocks_to_read);

    for (i = 0; i < number_blocks_to_read; i++) {

        printk(KERN_INFO "[FILE-onefilefs_read] index_blocks_to_read[%d] = %d \n", i, index_blocks_to_read[i]);
        block_to_read = index_blocks_to_read[i];

        bh = (struct buffer_head*)sb_bread(filp->f_path.dentry->d_inode->i_sb, block_to_read);
        if (!bh) {
            return -EIO;
        }

        memcpy(&value_size, bh->b_data, 2);

        printk(KERN_INFO "[FILE-onefilefs_read] block number = %d \n", block_to_read);
        printk(KERN_INFO "[FILE-onefilefs_read] value_size   = %hu \n", value_size);

        printk(KERN_INFO "[FILE-onefilefs_read] ---- message read ---- \n");
        printk("%s", bh->b_data + 2);
        printk(KERN_INFO "[FILE-onefilefs_read] ---- ---- \n");

        memcpy(data_user + len_data_user, bh->b_data + 2, value_size);
        len_data_user += value_size;

        if (i < number_blocks_to_read - 1) {
            memcpy(data_user + len_data_user, "\n", 1);
            len_data_user += 1;
        }

        printk(KERN_INFO "[FILE-onefilefs_read] ===== data_user [%d] ====== \n", i);
        printk(KERN_INFO "%s", data_user);

    }
    printk(KERN_INFO "[FILE-onefilefs_read] ===== data_user FINAL ====== \n");
    printk(KERN_INFO "%s", data_user);

    ret = copy_to_user(buf, data_user, len_data_user);

    return_value_read = len_data_user - ret;

    brelse(bh);
    kfree(data_user); // Free the memory allocated for the char_array 

    printk(KERN_INFO " ******************************** onefilefs_read ******************************** END \n");

    mutex_unlock(&my_mutex);

    return return_value_read;
}

struct dentry* onefilefs_lookup(struct inode* parent_inode, struct dentry* child_dentry, unsigned int flags) {

    struct onefilefs_inode* FS_specific_inode;
    struct super_block* sb = parent_inode->i_sb;
    struct buffer_head* bh = NULL;
    struct inode* the_inode = NULL;

    printk("[FILE-onefilefs_lookup] running the lookup inode-function for name %s \n", child_dentry->d_name.name);

    if (!strcmp(child_dentry->d_name.name, UNIQUE_FILE_NAME)) {

        // get a locked inode from the cache 
        the_inode = iget_locked(sb, 1);
        if (!the_inode)
            return ERR_PTR(-ENOMEM);

        // already cached inode - simply return successfully
        if (!(the_inode->i_state & I_NEW)) {
            return child_dentry;
        }

        // this work is done if the inode was not already cached
        inode_init_owner(sb->s_user_ns, the_inode, NULL, S_IFREG);

        the_inode->i_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IXUSR | S_IXGRP | S_IXOTH;
        the_inode->i_fop = &onefilefs_file_operations;
        the_inode->i_op = &onefilefs_inode_ops;

        // just one link for this file
        set_nlink(the_inode, 1);

        // now we retrieve the file size via the FS specific inode, putting it into the generic inode
        bh = (struct buffer_head*)sb_bread(sb, SINGLEFILEFS_INODES_BLOCK_NUMBER);
        if (!bh) {
            iput(the_inode);
            return ERR_PTR(-EIO);
        }
        FS_specific_inode = (struct onefilefs_inode*)bh->b_data;
        the_inode->i_size = FS_specific_inode->file_size;
        brelse(bh);

        d_add(child_dentry, the_inode);
        dget(child_dentry);

        // unlock the inode to make it usable 
        unlock_new_inode(the_inode);

        return child_dentry;
    }

    return NULL;
}

static int onefilefs_open(struct inode* inode, struct file* file) {

    // open the file in read only mode
    if ((file->f_flags & O_ACCMODE) == O_WRONLY || (file->f_flags & O_ACCMODE) == O_RDWR) {
        printk(KERN_INFO "[FILE-onefilefs_open] The file can only be opened for reading \n");
        return -EPERM;
    }

    printk(KERN_INFO "[FILE-onefilefs_open] The file has been opened in read mode successfully \n");
    return 0;
}

static int onefilefs_release(struct inode* inode, struct file* file) {

    printk(KERN_INFO "[FILE-onefilefs_release] The file has been released successfully \n");
    return 0;
}


// look up goes in the inode operations
const struct inode_operations onefilefs_inode_ops = {
    .lookup = onefilefs_lookup,
};

const struct file_operations onefilefs_file_operations = {
    .owner = THIS_MODULE,
    .open = onefilefs_open,
    .release = onefilefs_release,
    .read = onefilefs_read,
};
