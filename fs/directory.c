#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "directory.h"
#include "fat.h"
#include "../errors.h"

#define BITS_PER_BYTE   8

FILE_LIST* init_file_list()
{
    FILE_LIST* list = malloc(sizeof(FILE_LIST));
    list -> head = NULL;
    list -> dir_size = 0;
    return list;
}

FILE_NODE* init_file_node(const char* name, int size, unsigned char start_block)
{
    // copy name into a NAME_BYTES-sized malloc'd block
    char* fname = malloc(NAME_BYTES);
    strncpy(fname, name, NAME_BYTES);
    fname[NAME_BYTES-1] = NULL_TERM;

    // printf("  init_node: '%s'\n", fname);

    FILE_NODE* node = malloc(sizeof(FILE_NODE));
    node -> name = fname;
    node -> size = size;
    node -> start_block = start_block;
    node -> open_fds = 0;
    node -> marked_for_deletion = FALSE;
    node -> next = NULL;
    return node;
}

FILE_LIST* load_file_list(int fs_fd)
{
    // write_to_terminal("[get_encoding_size...]\n");
    unsigned int encoding_size = get_encoding_size(fs_fd);
    // write_to_terminal("[return init_file_list?]\n");
    if (encoding_size == 0) { return init_file_list(); }
    // write_to_terminal("[get_dir_enc...]\n");
    char* directory_encoding = get_directory_encoding(fs_fd, encoding_size);
    // write_to_terminal("[load_list...]\n");
    FILE_LIST* file_list = load_list_from_encoding(directory_encoding, encoding_size);
    free(directory_encoding);
    return file_list;
}

unsigned int get_encoding_size(int fs_fd)
{
    // TODO: handle errors from system calls
    unsigned char size_bytes[DIR_SIZE_BYTES];
    lseek_to_block(fs_fd, DIR_BLOCK, 0);
    read(fs_fd, size_bytes, DIR_SIZE_BYTES);
    // for (int i = 0; i < DIR_SIZE_BYTES; i++) { printf("[%d]: %d\n", i, size_bytes[i]); }
    int size = shift_chars_into_int(size_bytes, DIR_SIZE_BYTES);
    // printf("ENC_SIZE: %d\n", size);
    return size;
}

char* get_directory_encoding(int fs_fd, unsigned int encoding_size)
{
    char* encoding = malloc(encoding_size + 1);
    unsigned int cursor = DIR_SIZE_BYTES;

    read_from_file(
        fs_fd,
        DIR_BLOCK,
        encoding_size + DIR_SIZE_BYTES,
        (&cursor),
        encoding,
        encoding_size
    );

    encoding[encoding_size] = NULL_TERM;
    return encoding;
}

FILE_LIST* load_list_from_encoding(char* list_encoding, unsigned int encoding_size)
{
    FILE_LIST* list = init_file_list();
    FILE_NODE* prev = NULL;
    list -> head = NULL;

    // decode each node and append it to the list
    FILE_NODE* node;
    int num_nodes = encoding_size / FILE_ENC_BYTES;
    unsigned char* encoding = malloc(FILE_ENC_BYTES);
    for (int i = 0; i < num_nodes; i++) {
        memcpy(encoding, list_encoding + i * FILE_ENC_BYTES, FILE_ENC_BYTES);
        node = load_file_node_from_encoding(encoding);
        if (prev == NULL) { list -> head = node; }
        else { prev -> next = node; }
        prev = node;
    }

    free(encoding);
    list -> dir_size = encoding_size;
    return list;
}

FILE_NODE* load_file_node_from_encoding(unsigned char* node_encoding)
{
    // get name
    char* name = malloc(NAME_BYTES);
    memset(name, NULL_BYTE, NAME_BYTES);
    memcpy(name, node_encoding, NAME_BYTES);
    name[NAME_BYTES-1] = NULL_TERM;

    // get file size
    unsigned char* size_bytes = malloc(SIZE_BYTES);
    memset(size_bytes, NULL_BYTE, SIZE_BYTES);
    memcpy(size_bytes, node_encoding + NAME_BYTES, SIZE_BYTES);
    int size = shift_chars_into_int(size_bytes, SIZE_BYTES);
    free(size_bytes);

    // get start block
    unsigned char start_block = 
            (unsigned char) *(node_encoding + NAME_BYTES + SIZE_BYTES);

    FILE_NODE* new_node = init_file_node(name, size, start_block);
    free(name);
    return new_node;
}

FILE_NODE* find_node_with_name(FILE_LIST* list, const char* name)
{
    if (list == NULL || name == NULL) { return NULL; }
    for (FILE_NODE* curr = list -> head; curr != NULL; curr = curr -> next) {
        // printf("  find_node_with_name -- comparing: '%s' with '%s' ... ",
        //         curr -> name, name);
        if (strcmp(curr -> name, name) == 0) {
            // printf(" same\n");
            return curr;
        }
        // printf("\n");
    }
    return NULL;
}

void add_file(int fs_fd, FILE_LIST* list, FILE_NODE* node)
{
    // append file to list
    if (list -> head == NULL) { list -> head = node; }
    else {
        FILE_NODE* last = list -> head;
        while (last -> next != NULL)
            last = last -> next;
        last -> next = node;
    }

    // update FAT // TODO: this is already done in f_open.
    // which one do we keep?
    // unsigned char block = node -> start_block;
    // set_next_block(fs_fd, block, block);
    
    // update directory file
    unsigned int cursor = list -> dir_size + DIR_SIZE_BYTES;
    // printf("  add_file, cursor_before: %u\n", cursor);
    export_file(fs_fd, node, &cursor);
    // printf("  add_file, cursor_after: %u\n", cursor);
    update_dir_size(fs_fd, list, cursor - DIR_SIZE_BYTES);
}

unsigned int export_file_list(int fs_fd, FILE_LIST* list)
{
    unsigned int cursor = DIR_SIZE_BYTES;
    FILE_NODE* curr = list -> head;
    while (curr != NULL) {
        export_file(fs_fd, curr, &cursor);
        curr = curr -> next;
    }
    update_dir_size(fs_fd, list, cursor - DIR_SIZE_BYTES);
    return cursor;
}

unsigned int export_file(int fs_fd, FILE_NODE* node, unsigned int* cursor)
{
    // write_to_terminal("[Exporting file...]\n");
    // encode name
    unsigned char* encoding = malloc(FILE_ENC_BYTES+1);
    strncpy(encoding, node -> name, NAME_BYTES);
    encoding[NAME_BYTES-1] = NULL_TERM;
    
    // printf("  node->name: %s\n", node -> name);

    // write_to_terminal(encoding);

    // encode file size
    unsigned char size_bytes[SIZE_BYTES];
    unsigned int file_size = node -> size;
    // printf("  node->size: %d\n", node -> size);
    for (int i = SIZE_BYTES-1; i >= 0; i--) {
        size_bytes[i] = file_size & CHAR_MASK;
        // printf("    size_byte[%d] = %u\n", i, size_bytes[i]);
        file_size >>= BITS_PER_BYTE;
    }
    memcpy(encoding + NAME_BYTES, size_bytes, SIZE_BYTES);
    
    // encode start block
    encoding[NAME_BYTES + SIZE_BYTES] = node -> start_block;

    // write encoding to directory file
    int bytes_written = write_to_file(
        fs_fd,
        DIR_BLOCK,
        *cursor,
        cursor,
        encoding,
        FILE_ENC_BYTES
    );

    free(encoding);
    return bytes_written;
}

int read_from_file(int fs_fd, unsigned char start_block, unsigned int file_size,
        unsigned int* cursor, char* buf, unsigned int read_size)
{
    // return 0 if we can't read
    // TODO: are these errors
    if (read_size == 0) { return 0; }
    if (file_size <= (*cursor)) { return 0; }

    // write_to_terminal("[READ 1]\n");

    // find block that the cursor is on
    int curr_block_offset = (*cursor) % BLOCK_SIZE;
    unsigned char curr_block = start_block;
    for (int i = 0; i < (*cursor) / BLOCK_SIZE; i++) {
        curr_block = get_next_block(fs_fd, curr_block);
    }

    // write_to_terminal("[READ 2]\n");

    // initialize, and find how many bytes are left
    // in the current block (based on cursor pos)
    int bytes_read = 0;
    int bytes_left = 
            read_size < file_size - *cursor ? 
            read_size : file_size - *cursor;
    int bytes_to_read = 
            bytes_left <= (BLOCK_SIZE - curr_block_offset) ?
            bytes_left : (BLOCK_SIZE - curr_block_offset);

    // write_to_terminal("[READ 3]\n");
    
    // read until the end of the current block
    // or the end of the file, whichever comes first
    // repeat if there's still more to read
    while (bytes_to_read > 0) {
        // write_to_terminal("[READ 4]");
        lseek_to_block(fs_fd, curr_block, curr_block_offset);
        read(fs_fd, buf + bytes_read, bytes_to_read);
        bytes_read += bytes_to_read;
        bytes_left -= bytes_to_read;
        bytes_to_read = bytes_left <= BLOCK_SIZE ? bytes_left : BLOCK_SIZE;
        curr_block = get_next_block(fs_fd, curr_block);
        curr_block_offset = 0;
    }

    // write_to_terminal("[READ 5]\n");

    (*cursor) += bytes_read;
    return bytes_read;
}

void remove_file(FILE_LIST* file_list, FILE_NODE* file, int fs_fd)
{
    FILE_NODE* curr = file_list -> head;
    FILE_NODE* prev = NULL;
    while (curr != NULL) {
        if (curr == file) {
            if (prev == NULL) { file_list -> head = curr -> next; }
            else { prev -> next = curr -> next; }
            break;
        } else {
            prev = curr;
            curr = curr -> next;
        }
    }

    // free file node
    free_file_node(curr);

    // write linked list back to directory file
    export_file_list(fs_fd, file_list);
    
    // update FAT
    free_blocks(fs_fd, file -> start_block);
}

int write_to_file(int fs_fd, unsigned char start_block, unsigned int file_size, 
        unsigned int* cursor, const unsigned char* buf, unsigned int write_size)
{
    // return 0 if we can't write
    // TODO: are these errors?
    if (write_size == 0) { return 0; }

    // the size of the hole between the end of the file and the cursor
    int hole_size =
        file_size < (*cursor) ?
        (*cursor - file_size) : 0;

    // write_to_terminal("[WRITE 1]\n");

    // printf("  [hole_size: %d]\n", hole_size);

    // create new buffer with hole_size null bytes appended to the front
    int _write_size = hole_size + write_size;
    char* _buf = malloc(_write_size+1);
    memset(_buf, NULL_TERM, _write_size);
    memcpy(_buf + hole_size, buf, write_size);

    // where we start writing
    int write_start =
            file_size < (*cursor) ?
            file_size : (*cursor);
    int write_end;

    // write_to_terminal("[WRITE 2]\n");

    // find block that the cursor is on
    int curr_block_offset = write_start % BLOCK_SIZE;
    unsigned char next_block;
    unsigned char free_block;
    unsigned char curr_block = start_block;
    for (int i = 0; i < write_start / BLOCK_SIZE; i++) {
        next_block = get_next_block(fs_fd, curr_block);
        if (next_block == curr_block) {
            free_block = find_free_block(fs_fd);
            if (free_block == DIR_BLOCK) {
                set_errno(E_FAT_FULL);
                return -1;
            }
            next_block = free_block;
            set_next_block(fs_fd, curr_block, next_block);
            set_next_block(fs_fd, next_block, next_block);
        }
        curr_block = next_block;
    }

    // initialize, and find how many bytes are left
    // in the current block (based on cursor pos)
    int bytes_written = 0;
    int bytes_left = _write_size;
    int bytes_to_write = 
            bytes_left <= (BLOCK_SIZE - curr_block_offset) ?
            bytes_left : (BLOCK_SIZE - curr_block_offset);

    // write_to_terminal("[WRITE 3]\n");
    
    // write to the end of the current block
    // or the end of the file, whichever comes first
    // repeat if there's still more to write
    while (bytes_to_write > 0) {
        // write_to_terminal("[WRITE 4]\n");
        lseek_to_block(fs_fd, curr_block, curr_block_offset);
        write(fs_fd, _buf + bytes_written, bytes_to_write);
        bytes_written += bytes_to_write;
        bytes_left -= bytes_to_write;
        bytes_to_write = bytes_left <= BLOCK_SIZE ? bytes_left : BLOCK_SIZE;
        next_block = get_next_block(fs_fd, curr_block);

        // printf("[\nnext_block: %u\n", next_block);
        // printf("curr_block: %u\n", curr_block);
        // printf("bytes_to_write: %d\n]\n", bytes_to_write);
        // request another block for the file if needed
        if (next_block == curr_block && bytes_to_write > 0) {
            // write_to_terminal("Getting another block...\n");
            free_block = find_free_block(fs_fd);
            // printf("free block: %u\n", free_block);
            if (free_block == DIR_BLOCK) {
                break;
            } else {
                next_block = free_block;
                set_next_block(fs_fd, curr_block, next_block);
                set_next_block(fs_fd, next_block, next_block);
            }
        }

        curr_block = next_block;
        curr_block_offset = 0;
    }

    // write_to_terminal("[WRITE 5]\n");

    // printf(  "cursor b4: %d\n", *cursor);

    free(_buf);
    // printf(  "write_start: %d\n", write_start);
    // printf(  "bytes_written: %d\n", bytes_written);
    write_end = write_start + bytes_written;
    // printf(  "write_end: %d\n", write_end);
    (*cursor) = write_end;
    // printf(  "cursor after: %d\n", *cursor);
    return write_end - write_start;
}

void free_file_list(FILE_LIST* list)
{
    FILE_NODE* curr = list -> head;
    FILE_NODE* next = NULL;
    while (curr != NULL) {
        next = curr -> next;
        free_file_node(curr);
        curr = next;
    }
    free(list);
}

void free_file_node(FILE_NODE* node)
{
    if (node == NULL) { return; }
    free((char*) (node -> name));
    free(node);
}

int mark_file_for_deletion(FILE_NODE* file, FILE_LIST* file_list, int fs_fd)
{
    if (file -> open_fds == 0) {
        remove_file(file_list, file, fs_fd);
        return TRUE;
    } else {
        file -> marked_for_deletion = TRUE;
        return FALSE;
    }
}


void update_dir_size(int fs_fd, FILE_LIST* list, unsigned int new_dir_size)
{
    // printf("  updating dir_size: %d\n", new_dir_size);
    list -> dir_size = new_dir_size;
    unsigned char bytes[DIR_SIZE_BYTES];
    for (int i = DIR_SIZE_BYTES-1; i >= 0; i--) {
        bytes[i] = (unsigned char) (new_dir_size & CHAR_MASK);
        // printf("  bytes[%d]: %u\n", i, bytes[i]);
        new_dir_size >>= BITS_PER_BYTE;
    }

    // TODO: handle system call errors?
    lseek_to_block(fs_fd, DIR_BLOCK, 0);
    write(fs_fd, bytes, DIR_SIZE_BYTES);
}

unsigned int shift_chars_into_int(unsigned char* buf, unsigned int n)
{
    unsigned int num = 0;
    for (int i = 0; i < n; i++) {
        num <<= BITS_PER_BYTE;
        num |= buf[i];
    }
    return num;
}

void truncate_file(int fs_fd, FILE_LIST* list, FILE_NODE* file)
{
    file -> size = 0;
    unsigned char start = file -> start_block;
    free_blocks(fs_fd, start);
    set_next_block(fs_fd, start, start);
    export_file_list(fs_fd, list);
}
