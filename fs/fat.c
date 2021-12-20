#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fat.h"

void write_to_terminal(char* str)
{
    write(STDOUT_FILENO, str, strlen(str));
}

unsigned char get_next_block(int fs_fd, unsigned char index)
{   
    // TODO: handle errors on system calls
    unsigned char next_block;
    lseek(fs_fd, index, SEEK_SET);
    read(fs_fd, &next_block, 1);
    // printf("  getting [%d] -> %d\n", index, next_block);
    return next_block;
}

void set_next_block(int fs_fd, unsigned char index, unsigned char value)
{
    // TODO: handle errors on system calls
    // printf("  setting [%d] -> %d\n", index, value);
    lseek(fs_fd, index, SEEK_SET);
    write(fs_fd, &value, 1);
}

void lseek_to_block(int fs_fd, unsigned char block, unsigned int offset)
{
    // TODO: handle errors on system calls
    // printf("lseek: block:  (%d)\n", block);
    // printf("lseek: offset: (%d)\n", offset);
    // printf("lseek: index:  (%d)\n", (FAT_SIZE + (block * BLOCK_SIZE) + offset));
       
    lseek(fs_fd, FAT_SIZE + (block * BLOCK_SIZE) + offset, SEEK_SET);
}

unsigned char find_free_block(int fs_fd)
{
    // TODO: handle errors on system calls
    char* fat = malloc(FAT_SIZE * sizeof(char));
    lseek(fs_fd, 0, SEEK_SET);
    read(fs_fd, fat, FAT_SIZE);
    unsigned char free_block = DIR_BLOCK;
    // for (int i = 0; i < 10; i++) {
    //     printf("fat[%d] = %d\n", i, fat[i]);
    // }
    for (int i = 1; i < FAT_SIZE; i++) {
        if (fat[i] == FREE_BLOCK) {
            // printf("found_free_block: [%d] -> %d\n", i, fat[i]);
            free_block = (unsigned char) i;
            break;
        }
    }
    free(fat);
    return free_block;
}

void free_blocks(int fs_fd, unsigned char start_block)
{
    unsigned char curr_block;
    unsigned char next_block = start_block;
    do {
        curr_block = next_block;
        next_block = get_next_block(fs_fd, curr_block);
        set_next_block(fs_fd, curr_block, FREE_BLOCK);
    } while (curr_block != next_block);
}