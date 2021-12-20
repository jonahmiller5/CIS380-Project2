#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "../fs/fat.h"

#define ERROR_ARGS "usage: mkFlatFAT [FILENAME]\n"
#define OPEN_MODE   0644
#define INIT_CHAR   '\0'

int main(int argc, char const *argv[])
{
    // arg check
    if (argc != 2) {
        write(STDOUT_FILENO, ERROR_ARGS, sizeof(ERROR_ARGS));
        return EXIT_FAILURE;
    }

    // create FlatFAT file
    const char* file_path = argv[1];
    int fat_fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, OPEN_MODE);
    
    // init values of FAT table and blocks
    int file_size = FAT_SIZE + (FAT_SIZE * BLOCK_SIZE);
    char buffer[file_size];
    memset(buffer, INIT_CHAR, file_size);
    write(fat_fd, buffer, file_size);
    close(fat_fd);

    return EXIT_SUCCESS;
}
