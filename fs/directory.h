#ifndef _DIRECTORY_H
#define _DIRECTORY_H

/**
 * @brief The directory will start with 4 bytes (unsigned int),
 * denoting the length of the directory file itself, since we don't know
 * how long it is. 
 * 
 */
#define DIR_SIZE_BYTES  4

/**
 * @brief All files encodings are separated by a '|'
 * in the directory file
 * 
 * @deprecated
 */
#define FILE_DELIM      "|"

/**
 * @brief Each file encoding in the directory file 
 * has its fields separated by ','s
 * 
 * @deprecated
 */
#define FIELD_DELIM     ","

#define NAME_BYTES      256
#define SIZE_BYTES      3
#define START_BYTES     1
#define FILE_ENC_BYTES  (NAME_BYTES + SIZE_BYTES + START_BYTES)

#define NULL_TERM       '\0'
#define TRUE            1
#define FALSE           0
#define CHAR_MASK       0x000000FF

/**
 * @brief A FILE_NODE contains information about
 * a file in the filesystem, as well as a pointer to the
 * next file in the filesystem.
 * 
 */
typedef struct FILE_NODE {
    const char * name;
    unsigned int size;
    unsigned char start_block;
    unsigned int open_fds;
    int marked_for_deletion;
    struct FILE_NODE * next;
} FILE_NODE;

/**
 * @brief A FILE_LIST contains a pointer to 
 * the head of a FILE_NODE linked list, containing all
 * the files in the filesystem (not including the directory
 * file).
 * 
 */
typedef struct FILE_LIST {
    FILE_NODE *head;
    unsigned int dir_size;
} FILE_LIST;

/**
 * @brief Initializes a file list.
 * Note: Not to be confused with load_file_list, which
 * loads a file list from the filesystem file. This function
 * is just a helper function for that one.
 * 
 * @return FILE_LIST*
 */
FILE_LIST* init_file_list();

/**
 * @brief Initializes a file node with iven values.
 * 
 * @return FILE_NODE* 
 */
FILE_NODE* init_file_node(const char* name, int size, unsigned char start_block);

/**
 * @brief Parses the directory file and loads all
 * the file information into a FILE_LIST
 * 
 * @param fs_fd 
 * @return FILE_LIST* 
 */
FILE_LIST* load_file_list(int fs_fd);

/**
 * @brief Get the size of the directory encoding
 * 
 * @param fs_fd 
 * @return unsigned int 
 */
unsigned int get_encoding_size(int fs_fd);

/**
 * @brief Reads the first DIR_SIZE_BYTES of the 
 * first file in the filesystem and interprets them
 * as an integer denoting the length of the directory file.
 * 
 * 
 * @param fat_fd 
 * @return unsigned int
 */
unsigned int get_directory_file_size(int fs_fd);

/**
 * @brief Read the entire directory file starting AFTER
 * the first DIR_SIZE_BYTES bytes.
 * 
 * @param fs_fd 
 * @param size 
 * @return char* 
 */
char* get_directory_encoding(int fs_fd, unsigned int directory_size);

/**
 * @brief Loads the file list information encoded in 
 * list_encoding into a FILE_LIST.
 * Expected encoding:
 * "file1|file2|...|fileN"
 * 
 * @return FILE_LIST* 
 */
FILE_LIST* load_list_from_encoding(char* list_encoding, unsigned int encoding_size);

/**
 * @brief Loads the file information encoded in 
 * node_encoding into a FILE_NODE.
 * Expected encoding:
 * "name,size,start_block"
 * 
 * @param node_encoding 
 * @return FILE_NODE* 
 */
FILE_NODE* load_file_node_from_encoding(unsigned char* node_encoding);

/**
 * @brief Looks through the list to find a file node with a 
 * matching name. If match is found, return NULL.
 * 
 * @param list 
 * @param name 
 * @return FILE_NODE* 
 */
FILE_NODE* find_node_with_name(FILE_LIST* list, const char* name);

/**
 * @brief Add FILE_NODE* node to the end of the given file list.
 * This function will also update the FAT and append the new file
 * encoding to the directory file.
 * 
 * @param fs_fd
 * @param list 
 * @param node 
 */
void add_file(int fs_fd, FILE_LIST* list, FILE_NODE* node);

/**
 * @brief Encodes the file list into the directory file.
 * Returns the new length of the list encoding.
 * 
 * @param list
 * @return unsigned int - number of bytes written
 */
unsigned int export_file_list(int fs_fd, FILE_LIST* list);

/**
 * @brief Encode the file node and appends it to the
 * end of the directory file. 
 * 
 * @param fs_fd
 * @param node
 * @param cursor
 * @return unsigned int - number of bytes written
 */
unsigned int export_file(int fs_fd, FILE_NODE* node, unsigned int* cursor);

/**
 * @brief Read from the file that starts at start_block.
 * This function will update the value referenced by cursor to
 * its new spot in the file
 * 
 * @param fs_fd the filesystem file descriptor
 * @param start_block the start block of the file to read from
 * @param file_size the size of the file
 * @param cursor a pointer to the cursor offset
 * @param buf where to store the newly-read bytes
 * @param read_size the number of bytes to read
 * @return int - the number of bytes read
 */
int read_from_file(int fs_fd, unsigned char start_block, unsigned int file_size,
        unsigned int* cursor, char* buf, unsigned int read_size);

/**
 * @brief Removes the file from the file system and 
 * updates the FAT and directory.
 * 
 * @param file_list 
 * @param file 
 */
void remove_file(FILE_LIST* file_list, FILE_NODE* file, int fs_fd);

/**
 * @brief Write to a file starting at cursor.
 * 
 * @note Since this function does not have a reference to the FILE_NODE,
 * the caller must update the file_size themselves:
 *   file_size = max{cursor, file_size}
 * 
 * @param fs_fd 
 * @param start_block 
 * @param file_size 
 * @param cursor 
 * @param buf 
 * @param write_size 
 * @return int 
 */
int write_to_file(int fs_fd, unsigned char start_block, unsigned int file_size, 
        unsigned int* cursor, const unsigned char* buf, unsigned int write_size);

/**
 * @brief Frees the given file list.
 * 
 * @param the list to free
 */
void free_file_list(FILE_LIST* list);

/**
 * @brief Frees the given file node.
 * 
 * @param node the node to free
 */
void free_file_node(FILE_NODE* node);

/**
 * @brief Marks a file for deletion. If there are no open 
 * descriptors referencing this file, the file is removed.
 * 
 * @param file 
 * @param file_list
 * @return int
 */
int mark_file_for_deletion(FILE_NODE* file, FILE_LIST* file_list, int fs_fd);

/**
 * @brief Sets the dir_size field in the file list to new_dir_size.
 * Also writes the new dir size to the front of the dir file.
 * 
 * @param fs_fd
 * @param list 
 * @param new_dir_size 
 */
void update_dir_size(int fs_fd, FILE_LIST* list, unsigned int new_dir_size);

/**
 * @brief Shift chars into int. Used for decoding directory file
 * 
 * @param char 
 * @param n 
 * @return unsigned int 
 */
unsigned int shift_chars_into_int(unsigned char* buf, unsigned int n);

/**
 * @brief Sets the fils size to zero and free any blocks
 * it used to take up.
 * 
 * @param fs_fd
 * @param list
 * @param file
 */
void truncate_file(int fs_fd, FILE_LIST* list, FILE_NODE* file);

#endif