# ============================ PARAMETES ============================
NBLOCKS = 10# number of blocks chosen from user
# you must also set the NBLOCKS in "module_parameters.h" as this value

NBLOCKS_IMAGE = 100#number of blocks in the file image created
FILE_IMAGE = my_file_image# the file where is saved the file system metadata and the block device content
MOUNT_POINT = my_mount_point# directory where to mount the file system

# Find the number of the first free loopX device and save it in a variable
#		losetup: a command-line utility for setting up and configuring loop devices
#		-f:		 option tells losetup to find the first available loop device
#		grep:	 command is used to search for patterns within text. 
#		-o: 	 option tells grep to only output the matched part of the line 
#		-E:		 option enables extended regular expressions, used to search for a number consisting of one or more digits in the output of the "losetup -f" command
#		[0-9]+:  the pattern searches for one or more digits in the output
LOOP_NUM := $(shell losetup -f | grep -oE '[0-9]+')# take the first free loopX device
LOOPX := loop$(LOOP_NUM)# the free loop device selected
LOOPX_BUSY := $(shell  losetup -a | grep $(FILE_IMAGE) | grep -oE 'loop[0-9]+')# the loop device now busy

KVERSION := $(shell uname -r)# current kernel build version
PWD := $(CURDIR)# absolute pathname of the current working director

obj-m += the_usctm.o singlefilefs.o

the_usctm-objs := ./add_new_sc/usctm.o ./add_new_sc/vtpmo.o ./my_system_calls/bit_map.o ./my_system_calls/new_system_call.o
singlefilefs-objs := ./singlefile-FS/singlefilefs_src.o ./singlefile-FS/file.o ./singlefile-FS/dir.o ./my_system_calls/new_system_call.o ./my_system_calls/bit_map.o

# force rebuild for new tests: is it solves problems related to "make <<target>> is up to date"
.PHONY: test_operations

all:
# ============================ LOOP0 - BLOCK DEVICE ============================

# The loop device is not a file system. It is a device driver that makes a regular file accessible just like a block device.
# The reason this is useful is that file systems are built on top of block devices. 
# With the loop driver, we can give any file a block device interface and then construct a file system within the file.
# A loop device is a pseudo-device which doesn’t correspond to a real, physical block device, but can be used to make a file appear and be treated like one.

# Create the file that will be treated as a block device (used as the device storage)
# Create a file named FILE_IMAGE with a size of NBLOCKS_IMAGE*4096 
# 100*4096 = 409,600 bytes: 100 blocks of 4096 bytes each, all them filled with zeros (each block contains null bytes)
#		dd: 		   		 a command-line utility for copying and converting data
#		if=/dev/zero:  		 set the input file to /dev/zero, which is a special file that provides an endless stream of null bytes when read
#		of=FILE_IMAGE: 		 set the output file to FILE_IMAGE
#		bs=4096: 	   		 set the block size to 4096 bytes
#		count=NBLOCKS_IMAGE: set the number of blocks to copy to NBLOCKS_IMAGE
# ----> create_file_image:
# check that NBLOCKS <= NBLOCKS_IMAGE
ifeq ($(shell [ $(NBLOCKS) -lt $(NBLOCKS_IMAGE) ] && echo 1),1)
	$(info CASE 1 ----> NBLOCKS < NBLOCKS_IMAGE : $(NBLOCKS) < $(NBLOCKS_IMAGE))

else ifeq ($(shell [ $(NBLOCKS) -gt $(NBLOCKS_IMAGE) ] && echo 1),1)
	$(info CASE 2 ----> NBLOCKS > NBLOCKS_IMAGE : $(NBLOCKS) > $(NBLOCKS_IMAGE))
	$(error Must be: NBLOCKS <= NBLOCKS_IMAGE)

else
	$(info CASE 3 ----> NBLOCKS = NBLOCKS_IMAGE : $(NBLOCKS) = $(NBLOCKS_IMAGE))
endif
# create the image
	dd if=/dev/zero of=$(FILE_IMAGE) bs=4096 count=$(NBLOCKS_IMAGE)

# Save the first free loopX device in module_parameters.h
# ----> write_number_loopX:
#	@echo $(LOOP_NUM)
#	@echo $(LOOPX)
#		sed:					  Stream editor used for text manipulation. It allows you to modify files by applying certain patterns or expressions.
#		-i:						  Option used with the "sed" command to edit files in-place. When this option is used, the original file is modified directly without creating a backup.
#		's|pattern|replacement|': Substitute command in sed. It searches for a pattern within the file and replaces it with the provided replacement text.
# 		"/dev/loop[^"]*":		  consider everything that follows "/dev/loop" except the doble quote "
#		module_parameters.h:	  is the name of the file that will be modified by the sed command
	sed -i 's|#define MY_DEV_LOOPX "/dev/loop[^"]*"|#define MY_DEV_LOOPX "/dev/loop$(LOOP_NUM)"|' module_parameters.h

# Associate the file FILE_IMAGE with the loop device LOOPX (/dev/loop0)
# Map the file to a block device, then the LOOPX can now be used as a normal block device
# /dev/loop0 is a loop device that allows a regular file to be accessed as a block device
#		losetup: 	a command-line utility for setting up and configuring loop devices
#		/dev/LOOPX: the loop device to associate with the file
#		./my_file:  the file to associate with the loop device
# ----> link_file_to_block_device:
	sudo losetup /dev/$(LOOPX) ./$(FILE_IMAGE)

# ============================ DEVICE DRIVER CONFIGURATIONS - LOAD ============================

# compile all necessary files
# ----> all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
	gcc -Wall -pedantic -o singlefilemakefs ./singlefile-FS/singlefilemakefs.c

# load the modules
# ----> load:
# new system calls
	sudo insmod the_usctm.ko
# file system
	sudo insmod singlefilefs.ko
# dmesg | tail -50

# write the file system metadata
# ----> create_file_system:
	./singlefilemakefs $(FILE_IMAGE)

# mount the file system in the specified directory
#		mount: 			 command used to mount a file system on a directory
#		-t singlefilefs: option to specify the type of file system to be mounted
#		/dev/LOOPX:		 specify the device file to be mounted, which is a loop device that is used to mount a file as a block device
#		./MOUNT_POINT/:  specify the directory where the file system should be mounted
# ----> mount_file_system:
	mkdir $(MOUNT_POINT)
	sudo mount -t singlefilefs /dev/$(LOOPX) ./$(MOUNT_POINT)/

# ============================ DEVICE DRIVER TEST ============================

# if NBLOCKS = 4 	 	 (in Makefile and module_parameters.h)	∧ (NBLOCKS <= NBLOCKS_IMAGE) ----> test system calls
# if NBLOCKS = n ∈ N-{4} (in Makefile and module_parameters.h)	∧ (NBLOCKS <= NBLOCKS_IMAGE) ----> test VFS operations
test_operations:
	gcc -Wall -pedantic -o test_sc ./test/demo.c
	sudo ./test_sc $(NBLOCKS)
	rm test_sc
# dmesg | tail -150

# ============================ DEVICE DRIVER CONFIGURATIONS - UNLOAD ============================

# unload the modules
unload:
# new system calls
	sudo rmmod the_usctm.ko
# file system
	sudo umount ./$(MOUNT_POINT)/
	sudo rmmod singlefilefs.ko
# dmesg | tail -20

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
# new system calls
	@echo $(LOOPX_BUSY)
	sudo losetup --detach /dev/$(LOOPX_BUSY)
	rm ./$(FILE_IMAGE)
# file system
	rm singlefilemakefs
	rm -r $(MOUNT_POINT)

# ============================ INFO ============================

info_loop:
# List the loop devices existing in the system and get their status
	losetup
# List all devices associated with a file
	losetup --associated ./$(FILE_IMAGE)
# list information about all available block devices (--bytes: in bytes)
	lsblk
	lsblk --bytes

# print modules informations 
info_modules:
# lsmod needs the module file with .ko
	@echo "Module                  Size  Used by"
	lsmod | grep the_usctm
	lsmod | grep singlefilefs
	modinfo the_usctm.ko
	modinfo singlefilefs.ko

# print file system informations 
info_file_system:
	mount | grep singlefilefs
	findmnt