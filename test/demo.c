#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> // strlen()
#include <errno.h> // ENOMEM
#include <fcntl.h>

#include "../module_parameters.h"

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
*/

int execute_put_data(char* message_to_write, size_t bytes_to_write);
int execute_get_data(int index_block, char* message_to_read, size_t bytes_to_read);
int execute_invalidate_data(int index_block);

void test_system_calls(void);
void test_VFS_operations(void);

int main(int argc, char** argv) {

	// test_general();

	if (argc != 2) {
		printf("[DEMO] Usage: ./demo NBLOCKS \n");
		return -1;
	}

	if (strcmp(argv[1], "4") == 0) {

		printf("[DEMO] NBLOCKS = %s \n", argv[1]);
		test_system_calls();
	}
	else {
		printf("[DEMO] NBLOCKS = %s \n", argv[1]);
		test_VFS_operations();
	}

	printf(" ---- MAIN END ---- \n");
	return 0;
}

int execute_put_data(char* message_to_write, size_t bytes_to_write) {

	int index_block_written;

	index_block_written = syscall(134, message_to_write, bytes_to_write);
	printf("[execute_put_data] index_block_written = %d \n", index_block_written);

	return index_block_written;
}

int execute_get_data(int index_block, char* message_to_read, size_t bytes_to_read) {

	int number_bytes_read;

	number_bytes_read = syscall(174, index_block, message_to_read, bytes_to_read);
	printf("[execute_get_data] message_to_read   = %s \n", message_to_read);
	printf("[execute_get_data] number_bytes_read = %d \n", number_bytes_read);

	return number_bytes_read;
}

int execute_invalidate_data(int index_block) {

	int return_value;

	return_value = syscall(182, index_block);
	printf("[execute_invalidate_data] return_value = %d \n", return_value);

	return return_value;
}

// Makefile ---------------> NBLOCKS = 4
// module_parameters.h ----> NBLOCKS = 4 
void test_system_calls(void) {

	char  message_to_read[100] = "";
	int number_bytes_to_read = 2;

	int number_bytes_read;
	int index_block_empty = 10;

	int index_block_written;
	int index_block_to_invalidate;
	int return_value;

	printf(" =========== test general functionality put_data() - get_data() =========== \n");
	// write in first free block 
	index_block_to_invalidate = execute_put_data("HELLO_WORLD!", strlen("HELLO_WORLD!"));
	printf("[test_system_calls] message written: HELLO_WORLD!\n");

	// read from first free block
	execute_get_data(index_block_to_invalidate, message_to_read, number_bytes_to_read);

	printf(" =========== test ENOMEM in put_data() =========== \n");
	// write in block ?  but fail, there is not space
	index_block_written = execute_put_data("HELLO_WORLD!_2", strlen("HELLO_WORLD!_2"));
	printf("[test_system_calls] index_block_written = %d \n", index_block_written);

	if (index_block_written == ENOMEM) {
		printf("[test_system_calls] ENOMEM error \n");
	}

	printf(" =========== test ENODATA in get_data() =========== \n");
	strcpy(message_to_read, "");
	// read from block 10 but fail, there is not block 10
	number_bytes_read = execute_get_data(index_block_empty, message_to_read, number_bytes_to_read);
	printf("[test_system_calls] number_bytes_read = %d \n", number_bytes_read);

	if (number_bytes_read == ENODATA) {
		printf("[test_system_calls] ENODATA error \n");
	}

	printf(" =========== test general functionality invalidate_data() =========== \n");
	// invalidate the block selected
	execute_invalidate_data(index_block_to_invalidate);

	printf(" =========== test ENODATA in invalidate_data() =========== \n");
	// invalidate the block 10 but fail, there is not block 10
	return_value = execute_invalidate_data(index_block_empty);
	printf("[test_system_calls] return_value = %d \n", return_value);

	if (return_value == ENODATA) {
		printf("[test_system_calls] ENODATA error \n");
	}
}

// Makefile ---------------> NBLOCKS = n ∈ N - {4}
// module_parameters.h ----> NBLOCKS = n ∈ N - {4}
void test_VFS_operations(void) {

	int fd;
	int bytes_successfully_read;
	int index_block_to_invalidate;

	int bytes_to_read = SIZE_UNIQUE_FILE;
	char buff[bytes_to_read];

	int index_bi_1;
	int index_bi_2;
	int index_bi_4;

	// write in the blocks
	index_bi_1 = execute_put_data("<HELLO WORLD! - block B_1>", strlen("<HELLO WORLD! - block B_1>"));
	index_bi_2 = execute_put_data("<HELLO WORLD! - block B_2>", strlen("<HELLO WORLD! - block B_2>"));
	index_block_to_invalidate = execute_put_data("<HELLO WORLD! - block B_3>", strlen("<HELLO WORLD! - block B_3>"));
	index_bi_4 = execute_put_data("<HELLO WORLD! - block B_4>", strlen("<HELLO WORLD! - block B_4>"));

	// open the unique file
	fd = open(PATH_UNIQUE_FILE_NAME, O_RDONLY);
	if (fd < 0) {
		perror("[test_VFS_operations] error in open");
		exit(EXIT_FAILURE);
	}
	printf("[test_VFS_operations] opened the fd = %d \n", fd);

	// read content of the unique file
	bytes_successfully_read = read(fd, buff, bytes_to_read);
	printf("[test_VFS_operations] bytes_successfully_read = %d \n", bytes_successfully_read);

	printf(" ======================== \n");
	printf("%.*s\n", bytes_successfully_read, buff);
	printf(" ======================== \n");

	// invalidate a block in the unique file
	execute_invalidate_data(index_block_to_invalidate);

	// read new content of the unique file	
	strcpy(buff, "");

	bytes_successfully_read = read(fd, buff, bytes_to_read);
	printf("[test_VFS_operations] bytes_successfully_read = %d \n", bytes_successfully_read);

	printf(" ======================== \n");
	printf("%.*s\n", bytes_successfully_read, buff);
	printf(" ======================== \n");

	// close the unique file
	if (close(fd) < 0) {
		perror("[test_VFS_operations] error in close");
		exit(EXIT_FAILURE);
	}
	printf("[test_VFS_operations] closed the fd \n");

	// clean the unique file
	execute_invalidate_data(index_bi_1);
	execute_invalidate_data(index_bi_2);
	execute_invalidate_data(index_bi_4);
	printf("[test_VFS_operations] clean completed \n");
}

void test_general(void) {

	int index_block_written;
	char message_to_read[100] = "";
	int number_bytes_to_read = 2;

	int fd;
	int bytes_successfully_read;
	int bytes_to_read = SIZE_UNIQUE_FILE;
	char buff[bytes_to_read];

	index_block_written = execute_put_data("HELLO_WORLD!", strlen("HELLO_WORLD!"));

	execute_get_data(index_block_written, message_to_read, number_bytes_to_read);

	// -------------------------------------

	// open the unique file
	fd = open(PATH_UNIQUE_FILE_NAME, O_RDONLY);
	if (fd < 0) {
		perror("[test_VFS_operations] error in open");
		exit(EXIT_FAILURE);
	}
	printf("[test_VFS_operations] opened the fd = %d \n", fd);

	// read content of the unique file
	bytes_successfully_read = read(fd, buff, bytes_to_read);
	printf("[test_VFS_operations] bytes_successfully_read = %d \n", bytes_successfully_read);

	printf(" ======================== \n");
	printf("%.*s\n", bytes_successfully_read, buff);
	printf(" ======================== \n");

	// close the unique file
	if (close(fd) < 0) {
		perror("[test_VFS_operations] error in close");
		exit(EXIT_FAILURE);
	}
	printf("[test_VFS_operations] closed the fd \n");

	// clean the unique file
	execute_invalidate_data(index_block_written);
	printf("[test_VFS_operations] clean completed \n");
}

