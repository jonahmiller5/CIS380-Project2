#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "../kernel_functions.h"
#include "../errors.h"

#include "filesystem.h"
#include "descriptors.h"
#include "fat.h"

// ================== //
int TEST_MODE = TRUE; //
FD_LIST* test_list;   //
// ================== //

FILE_LIST* file_list;
int fs_fd;

void init_filesystem(const char* fs_name)
{
    fs_fd = open(fs_name, O_RDWR, OPEN_MODE);
    // write_to_terminal("[INIT_FILE_LIST...]\n");
    file_list = load_file_list(fs_fd);
    // write_to_terminal("[INIT_FD_LIST...]\n");
    if (TEST_MODE) { test_list = init_fd_list(file_list); }
}

void free_filesystem()
{
    // write_to_terminal("[FREE FD_LIST...]\n");
    // write_to_terminal("[FREE FILE_LIST...]\n");
    free_file_list(file_list);
    // write_to_terminal("[FREE CLOSE FD...]\n");
    close(fs_fd);
}

FILE_LIST* get_file_list()
{
    return file_list;
}

int f_open(const char * fname, int mode)
{
    FD_LIST* fd_list;
    if (TEST_MODE) {
        fd_list = test_list;
    } else {
        info* p_info = k_get_current_info();
        fd_list = p_info -> fd_list;
        free_info(p_info);
    }
    
    if (fname == NULL || *fname == NULL_TERM) { 
        set_errno(E_INVALID_FNAME);
        return F_FAILED;
    }
    
    // check for invalid modes
    if (mode != F_APPEND && mode != F_WRITE && mode != F_READ) {
        set_errno(E_INVALID_MODE);
        return F_FAILED;
    }

    // look for an existing file
    // write_to_terminal("[LOOK FOR EXISTING FILE...]\n");
    FILE_NODE* file = find_node_with_name(file_list, fname);
    if (file == NULL) { 
        if (mode == F_READ) {
            set_errno(E_FILE_NOT_FOUND);
            return F_FAILED;
        }
        
        unsigned char free_block = find_free_block(fs_fd);
        if (free_block == DIR_BLOCK) {
            set_errno(E_FAT_FULL);
            return F_FAILED;
        }

        set_next_block(fs_fd, free_block, free_block);
        file = init_file_node(fname, 0, free_block);
        add_file(fs_fd, file_list, file);
    }

    // get a new fd
    // write_to_terminal("[GET NEW FD...]\n");
    int fd = get_new_fd(file_list, fd_list, file, mode);
    // printf("  got_new_fd: %d\n", fd);
    if (fd == -1) {
        set_errno(E_MAX_FDS_OPEN);
        return F_FAILED;
    }

    // move cursor based on mode
    FD_NODE* fd_node = get_node_by_fd(fd, fd_list);
    if (mode == F_APPEND) {
        fd_node -> cursor = file -> size;
    } else if (mode == F_WRITE) { 
        truncate_file(fs_fd, file_list, file);
    }

    return fd;
}

int f_read(int fd, char * buf, unsigned int n)
{
    FD_LIST* fd_list;
    int fg;
    if (TEST_MODE) {
        fd_list = test_list;
        fg = FG;
    } else {
        info* p_info = k_get_current_info();
        fd_list = p_info -> fd_list;
        fg = (p_info -> ground == FG);
        free_info(p_info);
    }

    // find fd_node with matching fd
    FD_NODE* fd_node = get_node_by_fd(fd, fd_list);
    if (fd_node == NULL) {
        set_errno(E_FD_NOT_FOUND);
        return F_FAILED;
    }

    // TODO: set errno inside read_from_file ?
    char* fname = fd_node -> fname;
    FILE_NODE* file = find_node_with_name(file_list, fname);

    if (file == NULL) {
        if (fd == F_STDIN)  {
            if (!fg) {
                // TODO: send signal
            } else {
                return read(STDIN_FILENO, buf, n);
            }
        }
        if (fd == F_STDOUT) { return read(STDOUT_FILENO, buf, n); }
        if (fd == F_STDERR) { return read(STDERR_FILENO, buf, n); }
    }

    return read_from_file(
        fs_fd,
        file -> start_block,
        file -> size,
        &(fd_node -> cursor),
        buf,
        n
    );
}

// void f_print_fds() {
//     for (int j = 0; j < 64; j++){
//         printf("%d\n", get_node_by_fd(j, test_list));
//     }
// }

int f_write(int fd, const char * str, unsigned int n)
{
    // write_to_terminal("INPUT STR: ");
    // write_to_terminal(str);
    // write_to_terminal("\n");

    FD_LIST* fd_list;
    if (TEST_MODE) {
        fd_list = test_list;
        // printf("in test mode\n");
        // if (test_list != NULL) {
        //     printf("test_list is not NULL\n");
        // } else {
        //     printf("test_list is NULL\n");
        // }
    } else {
        info* p_info = k_get_current_info();
        fd_list = p_info -> fd_list;
        free_info(p_info);
    }

    // write_to_terminal("  [F_WRITE: finding node]\n");

    // for (int i = 0; i < 3; i++) {
    //     FD_NODE* fd_node = get_node_by_fd(i, fd_list);
    //     printf("fd file: %d %s\n", i, (fd_node -> file) -> name);
    // }

    // find fd_node with matching fds

    FD_NODE* fd_node = get_node_by_fd(fd, fd_list);
    if (fd_node == NULL) {
        // printf("FD NODE NULL !!!!!");
        set_errno(E_FD_NOT_FOUND);
        return F_FAILED;
    }

    // write_to_terminal("  [F_WRITE: write_to_file]\n");
    // write to file
    char* fname = fd_node -> fname;
    FILE_NODE* file = find_node_with_name(file_list, fname);
    
    // printf("fd: %d\n", fd);
    // if (file != NULL) {
    //     write_to_terminal("1\n");
    //     if (file->size == NULL){

    //         write_to_terminal("FUCKKKKK");
    //     } 
    //     if (file -> size == 0) {
    //         write_to_terminal("---\n");
    //     } else {
    //         write_to_terminal("===\n");
    //     }
    // } else {
    //     write_to_terminal("2");
    //     printf("ITS NULL????\n");
    // }
    

    if (file == NULL) {
        // write_to_terminal("3\n");
        // printf("FILE IS NULL, fd: %d\n", fd);
        if (fd == F_STDIN)  { return write(STDIN_FILENO, str, n); }
        if (fd == F_STDOUT) { return write(STDOUT_FILENO, str, n); }
        if (fd == F_STDERR) { return write(STDERR_FILENO, str, n); }
    }

    // write_to_terminal("GOT PAst STDOUTS\n");

    if (fd_node -> mode == F_READ) {
        set_errno(E_CANT_WRITE);
        return F_FAILED;
    }

    int bytes_written = write_to_file(
        fs_fd,
        file -> start_block,
        file -> size,
        &(fd_node -> cursor),
        str,
        n
    );

    // update file size
    file -> size =
        file -> size > fd_node -> cursor ?
        file -> size : fd_node -> cursor;

    // write_to_terminal("  [F_WRITE: export list]\n");
    export_file_list(fs_fd, file_list);
    return bytes_written;
}

int f_close(int fd)
{
    // write_to_terminal("one1\n");
    FD_LIST* fd_list;
    if (TEST_MODE) {
        fd_list = test_list;
    } else {
        info* p_info = k_get_current_info();
        fd_list = p_info -> fd_list;
        free_info(p_info);
    }
    
    // write_to_terminal("two2\n");
    // find FD_NODE to remove
    FD_NODE* fd_node = get_node_by_fd(fd, fd_list);
    if (fd_node == NULL) {
        set_errno(E_FD_NOT_FOUND);
        return F_FAILED;
    }
    // write_to_terminal("three3\n");

    // remove it
    char* fname = fd_node -> fname;
    FILE_NODE* file = find_node_with_name(file_list, fname);
    if (remove_fd_node(fd, fd_list) && file != NULL) {
        // check if the file was marked for deletion
        // i.e. was it waiting for this fd to be closed?
        if (file -> marked_for_deletion &&
                --(file -> open_fds) <= 0) {
            remove_file(file_list, file, fs_fd);
        }
    }

    return F_SUCCESS;
}

int f_unlink(const char * fname)
{
    FD_LIST* fd_list;
    if (TEST_MODE) {
        fd_list = test_list;
    } else {
        info* p_info = k_get_current_info();
        fd_list = p_info -> fd_list;
        free_info(p_info);
    }
    
    FILE_NODE* file = find_node_with_name(file_list, fname);
    if (file == NULL) {
        set_errno(E_FILE_NOT_FOUND);
        return F_FAILED;
    }

    if (!mark_file_for_deletion(file, file_list, fs_fd)) {
        set_errno(E_FILE_IN_USE);
        return F_FAILED;
    }
    export_file_list(fs_fd, file_list);
    return F_SUCCESS;
}

int f_lseek(int fd, int offset, int whence)
{
    FD_LIST* fd_list;
    if (TEST_MODE) {
        fd_list = test_list;
    } else {
        info* p_info = k_get_current_info();
        fd_list = p_info -> fd_list;
        free_info(p_info);
    }
    
    FD_NODE* fd_node = get_node_by_fd(fd, fd_list);
    if (fd_node == NULL) {
        set_errno(E_FD_NOT_FOUND);
        return F_FAILED;
    }

    char* fname = fd_node -> fname;
    FILE_NODE* file = find_node_with_name(file_list, fname);
    if (whence == F_SEEK_SET) { fd_node -> cursor = offset; }
    if (whence == F_SEEK_CUR) { fd_node -> cursor += offset; }
    if (whence == F_SEEK_END) { fd_node -> cursor = file -> size + offset; }
    return F_SUCCESS;
}

int f_dup2(int old_fd, int new_fd)
{
    if (old_fd < 0 || old_fd > FD_LIST_SIZE ||
        new_fd < 0 || new_fd > FD_LIST_SIZE) {
        printf("F DUP FD OUT OF RANGE"); // TODO
        return F_FAILED;
    }

    FD_LIST* fd_list;
    if (TEST_MODE) {
        fd_list = test_list;
    } else {
        info* p_info = k_get_current_info();
        fd_list = p_info -> fd_list;
        free_info(p_info);
    }
    
    FD_NODE* old_fd_node = get_node_by_fd(old_fd, fd_list);
    if (old_fd_node == NULL) { 
        set_errno(E_FD_NOT_FOUND);
        return F_FAILED;
    }

    // for (int i = 0; i <64; i ++){
    //     if ((fd_list->fds) [i] == NULL){
    //         printf("NULL\n");
    //     } else {
    //         printf("NOT NULL\n");
    //     }
    // }

    FD_NODE* new_fd_node = get_node_by_fd(new_fd, fd_list);
    if (old_fd_node == new_fd_node) { return new_fd; }

    // write_to_terminal("about to f_close\n");
    //close new "if necessary" according to man page
    if (new_fd_node != NULL) { f_close(new_fd); }
    
    set_fd_node(new_fd, fd_list, old_fd_node);
    return new_fd;
}

void f_dup_reset()
{
    FD_LIST* fd_list;
    if (TEST_MODE) {
        fd_list = test_list;
    } else {
        info* p_info = k_get_current_info();
        fd_list = p_info -> fd_list;
        free_info(p_info);
    }

    // close all
    f_close(F_STDIN);
    f_close(F_STDOUT);
    f_close(F_STDERR);

    // create new
    FD_NODE** fds = fd_list -> fds;
    fds[F_STDIN] = create_dummy_fd_node(NULL);
    fds[F_STDOUT] = create_dummy_fd_node(NULL);
    fds[F_STDERR] = create_dummy_fd_node(NULL);
}

void f_cat()
{
    // read and write until no bytes are left
    unsigned char* buffer[CAT_BUFFER_SIZE];
    memset(buffer, NULL_TERM, CAT_BUFFER_SIZE);
    int bytes_read;
    while ((bytes_read = f_read(F_STDIN, buffer, CAT_BUFFER_SIZE)) > 0) {
        if (bytes_read == -1) {
            p_perror("cat");
            return;
        }
        if (f_write(F_STDOUT, buffer, bytes_read) == -1) {
            p_perror("cat");
            return;
        }
    }
}

void f_ls()
{
    char int_buffer[INT_BUFF_LEN];

    f_write(F_STDOUT, LS_HEADER, sizeof(LS_HEADER));

    for (FILE_NODE* curr = file_list -> head; 
            curr != NULL; curr = curr -> next) {

        // write name
        f_write(F_STDOUT, curr -> name, sizeof(curr -> name));
        f_write(F_STDOUT, TAB_CHAR, sizeof(TAB_CHAR));

        // write size
        memset(int_buffer, NULL_TERM, INT_BUFF_LEN);
        sprintf(int_buffer, "%d", curr -> size);
        f_write(F_STDOUT, int_buffer, sizeof(int_buffer));
        f_write(F_STDOUT, TAB_CHAR, sizeof(TAB_CHAR));
        
        // write start block
        memset(int_buffer, NULL_TERM, INT_BUFF_LEN);
        sprintf(int_buffer, "%d", curr -> start_block);
        f_write(F_STDOUT, int_buffer, sizeof(int_buffer));
        f_write(F_STDOUT, NEWLINE, sizeof(NEWLINE));
    }
}

void f_touch(char* fname)
{
    int fd = f_open(fname, F_APPEND);
    if (fd == -1) {
        p_perror("touch");
        return;
    }
    if (f_close(fd) == -1) {
        p_perror("touch");
        return;
    }
}

void f_rm(char* fname)
{
    if (f_unlink(fname) == F_FAILED) {
        p_perror("rm");
        return;
    }
}

void f_reset()
{
    for (FILE_NODE* curr = file_list -> head; 
            curr != NULL;) {

        FILE_NODE* next = curr -> next;
        if (f_unlink(curr -> name) == F_FAILED) {
            p_perror("reset");
            return;
        }
        curr = next;
    }
}

void f_randchars(int size)
{
    // arg check
    if (size < RC_MIN_SIZE) { size = RC_MIN_SIZE; }
    if (size > RC_MAX_SIZE) { size = RC_MAX_SIZE; }

    // init rand
    time_t t;
    unsigned char buffer[size];
    srand((unsigned) time(&t));
    
    // create rand buffer
    for (int i = 0; i < size; i++) {
        buffer[i] = (unsigned char) ((rand()
                % (RC_MAX_CHAR - RC_MIN_CHAR)) + RC_MIN_CHAR);
    }

    // write rand buffer
    if ((f_write(F_STDOUT, buffer, size)) == F_FAILED) {
        p_perror("randchars");
        return;
    }
}

void p_perror(const char* prefix)
{
    char err[ERROR_MSG_SIZE];

    int curr_errno = get_errno();
    switch (curr_errno) {
        case E_INVALID_MODE:
            strncpy(err, M_INVALID_MODE, ERROR_MSG_SIZE);
            break;
        case E_FILE_NOT_FOUND:
            strncpy(err, M_FILE_NOT_FOUND, ERROR_MSG_SIZE);
            break;
        case E_FAT_FULL:
            strncpy(err, M_FAT_FULL, ERROR_MSG_SIZE);
            break;
        case E_INVALID_FNAME:
            strncpy(err, M_INVALID_FNAME, ERROR_MSG_SIZE);
            break;
        case E_MAX_FDS_OPEN:
            strncpy(err, M_MAX_FDS_OPEN, ERROR_MSG_SIZE);
            break;
        case E_FD_NOT_FOUND:
            strncpy(err, M_FD_NOT_FOUND, ERROR_MSG_SIZE);
            break;
        case E_CANT_WRITE:
            strncpy(err, M_CANT_WRITE, ERROR_MSG_SIZE);
            break;
        case E_FILE_IN_USE:
            strncpy(err, M_FILE_IN_USE, ERROR_MSG_SIZE);
            break;
    }

    // write error
    f_write(F_STDERR, prefix, strlen(prefix));
    f_write(F_STDERR, ERROR_SEP, strlen(ERROR_SEP));
    f_write(F_STDERR, err, strlen(err));
}
