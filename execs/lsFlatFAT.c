#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../fs/filesystem.h"
#include "../fs/descriptors.h"
#include "../fs/directory.h"
#include "../fs/fat.h"

#define ERROR_ARGS      "usage: lsFlatFAT [FILENAME]\n"
#define TAB_CHAR        "\t\t"
#define NEWLINE         "\n"
#define INT_BUFF_LEN    20

// void write_to_terminal();

int main(int argc, char const *argv[])
{
    // arg check
    if (argc != 2) {
        write(STDOUT_FILENO, ERROR_ARGS, sizeof(ERROR_ARGS));
        return EXIT_FAILURE;
    }

    // open file
    const char* file_path = argv[1];
    init_filesystem(file_path);

    // temp -- print the FAT
    int fs_fd = open(file_path, O_RDONLY, 0644);
    unsigned char* fat = malloc(FAT_SIZE+1);
    read(fs_fd, fat, FAT_SIZE);
    *(fat+FAT_SIZE) = NULL_TERM;
    for (int i = 0; i < 10; i++) {
        int num = shift_chars_into_int(fat+i, 1);
        // printf("%d: %d\n", i, num);
    }
    write_to_terminal(fat);
    free(fat);
    // -----
    
    char int_buffer[INT_BUFF_LEN];

    write_to_terminal("name\t\tsize\t\tstart\n");

    FILE_LIST* file_list = get_file_list();
    for (FILE_NODE* curr = file_list -> head; 
            curr != NULL; curr = curr -> next) {

        //write_to_terminal("[PRINT FILE:]\n");
        // write name
        write_to_terminal(curr -> name);
        write_to_terminal(TAB_CHAR);

        // write size
        memset(int_buffer, NULL_TERM, INT_BUFF_LEN);
        sprintf(int_buffer, "%d", curr -> size);
        write_to_terminal(int_buffer);
        write_to_terminal(TAB_CHAR);
        
        // write start block
        memset(int_buffer, NULL_TERM, INT_BUFF_LEN);
        sprintf(int_buffer, "%d", curr -> start_block);
        write_to_terminal(int_buffer);
        write_to_terminal(NEWLINE);
    }

    free_filesystem();
    return EXIT_SUCCESS;
}

// void write_to_terminal(char* str)
// {
//     write(STDOUT_FILENO, str, sizeof(str));
// }