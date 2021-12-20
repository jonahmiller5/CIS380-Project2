#ifndef _FAT_H
#define _FAT_H

#define FAT_SIZE    256
#define BLOCK_SIZE  1024
#define FREE_BLOCK  0
#define DIR_BLOCK   0
#define NULL_BYTE   0

void write_to_terminal(char* str);

/**
 * @brief Get the block at the specified index
 * 
 * @param fs_fd
 * @param index 
 * @return unsigned char 
 */
unsigned char get_next_block(int fs_fd, unsigned char index);

/**
 * @brief Set the value at the specified index
 * 
 * @param fs_fd
 * @param index 
 * @param value 
 */
void set_next_block(int fs_fd, unsigned char index, unsigned char value);

/**
 * @brief lseeks to the offset of the block in the fs file
 * 
 * @param fs_fd
 * @param block 
 * @param offset [0, BLOCK_SIZE) -- offset within the block
 */
void lseek_to_block(int fs_fd, unsigned char block, unsigned int offset);

/**
 * @brief Returns the fist block after 0 that maps to a 0.
 * If no such block is found, 0 is returned.
 * 
 * @param fs_fd
 * @return unsigned char 
 */
unsigned char find_free_block(int fs_fd);

/**
 * @brief Starting at start_block, follow all next-block
 * pointers, setting blocks to FREE_BLOCK in your wake, until
 * a block points to itself.
 * 
 * @param fs_fd 
 * @param start_block 
 */
void free_blocks(int fs_fd, unsigned char start_block);

#endif