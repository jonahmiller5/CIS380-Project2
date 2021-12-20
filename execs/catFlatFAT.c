#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../fs/directory.h"
#include "../fs/filesystem.h"
#include "../fs/descriptors.h"
#include "../fs/fat.h"

#define ERROR_ARGS  "usage: catFlatFAT [FILESYSTEM] [FILENAME] [-r|-w|-a]\n"
#define BUFFER_SIZE 4096

// void write_to_terminal();

int main(int argc, char const *argv[])
{
    if (argc != 4) {
        write_to_terminal(ERROR_ARGS);
        return EXIT_FAILURE;
    }

    // init filesystem
    const char* fs_path = argv[1];
    // write_to_terminal("[INIT FS...]\n");
    init_filesystem(fs_path);
    // write_to_terminal("[INIT DONE]\n");

    int mode = -1;
    const char* mode_str = argv[3];
    if (strcmp(mode_str, "-r") == 0) { mode = F_READ; }
    if (strcmp(mode_str, "-w") == 0) { mode = F_WRITE; }
    if (strcmp(mode_str, "-a") == 0) { mode = F_APPEND; }
    if (mode == -1) { write_to_terminal(argv[3]); }

    const char* file_path = argv[2];
    // write_to_terminal("[F_OPEN...]\n");
    int fd = f_open(file_path, mode);
    // write_to_terminal("[F_OPEN DONE]\n");

    char buffer[BUFFER_SIZE];
    if (mode == F_WRITE || mode == F_APPEND) {
        for (;;) {
            memset(buffer, NULL_TERM, BUFFER_SIZE-1);
            // write_to_terminal("[READING...]\n");
            int bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            // write_to_terminal("[DONE READ]\n");
            if (bytes_read == 0) { break; }
            // printf("%d\n", fd);
            f_write(fd, buffer, bytes_read);
        }
    } else {
        memset(buffer, NULL_TERM, BUFFER_SIZE-1);
        f_read(fd, buffer, BUFFER_SIZE-1);
        buffer[BUFFER_SIZE-1] = NULL_TERM;
        write(STDOUT_FILENO, buffer, BUFFER_SIZE);
    }    

    // write_to_terminal("[F_CLOSE...]\n");
    f_close(fd);
    // write_to_terminal("[FREE START]\n");
    free_filesystem();
    // write_to_terminal("[FREE END]\n");
    return EXIT_SUCCESS;
}

// void write_to_terminal(char* str)
// {
//     write(STDOUT_FILENO, str, strlen(str));
// }
