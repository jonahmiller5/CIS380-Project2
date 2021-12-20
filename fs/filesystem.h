#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

// constants for f_open
#define F_WRITE         1 // 001
#define F_READ          3 // 011
#define F_APPEND        7 // 111

// constants for f_lseek
#define F_SEEK_SET      1 // 001
#define F_SEEK_CUR      3 // 011
#define F_SEEK_END      7 // 111

// constants for STD vals
#define F_STDIN         0
#define F_STDOUT        1
#define F_STDERR        2

// relevant characters and strings
#define TAB_CHAR        "\t\t"
#define NEWLINE         "\n"
#define INT_BUFF_LEN    20
#define LS_HEADER       "name\t\tsize\t\tstart\n"
#define CAT_BUFFER_SIZE 4096

// permissions to open fs file
#define OPEN_MODE       0644

// return value on error
#define F_FAILED        -1
#define F_SUCCESS       0

// constants for shell built-ins
#define RC_MIN_SIZE     0
#define RC_MAX_SIZE     10000
#define RC_MIN_CHAR     33
#define RC_MAX_CHAR     125

/**
 * @brief Inits the global fields in filesystem.c that all other
 * functions depend on. Also opens the filesystem file for reading
 * and writing.
 * 
 * @param fs_name the file path of the filesystem file
 */
void init_filesystem(const char* fs_name);

/**
 * @brief Frees all data structures related to the filesystem.
 * 
 */
void free_filesystem();

/**
 * @brief open a file name fname with the mode mode and return a file
 * descriptor on success and a negative value on error.
 * 
 * additionally, the file pointer references the end of the file. 
 * 
 * @param fname 
 * @param mode The allowed modes are as follows:
 *   F_WRITE - writing and reading, truncates file, or creates one if needed;
 *   F_READ - open the file for reading only, return error if there's no file;
 *   F_APPEND - open the file for reading and writing but does not truncate; 
 * @return int 
 */
int f_open(const char * fname, int mode);

/**
 * @brief read n bytes from the file referenced by fd. On return, f_read
 * returns the number of bytes read, 0 if EOF is reached, or a negative number
 * on error.
 * 
 * @param fd 
 * @param n 
 * @param buf 
 * @return int 
 */
int f_read(int fd, char * buf, unsigned int n);

void f_print_fds();

/**
 * @brief write n bytes of the string referenced by str to the file fd and
 * increment the file pointer by n. On return, f_write returns the number of
 * bytes written, or a negative value on error.
 * 
 * @param fd 
 * @param str 
 * @param n 
 * @return int 
 */
int f_write(int fd, const char * str, unsigned int n);

/**
 * @brief close the file fd and return 0 on success, or a negative value on 
 * failure.
 * 
 * @param fd 
 * @return int 
 */
int f_close(int fd);

/**
 * @brief remove the file.
 * 
 * @param fname 
 */
int f_unlink(const char * fname);

/**
 * @brief reposition the file pointer for fd to the offset relative to whence. 
 * 
 * @param fd 
 * @param offset 
 * @param whence The allowed whences are as follows:
 *   F_SEEK_SET - The file offset is set to offset bytes.
 *   F_SEEK_CUR - The file offset is set to its current location plus offset bytes. 
 *   F_SEEK_END - The file offset is set to the size of the file plus offset bytes.
 */
int f_lseek(int fd, int offset, int whence);

/**
 * @brief dooop
 * 
 * @param old_fd 
 * @param new_fd 
 * @return int 
 */
int f_dup2(int old_fd, int new_fd);

/**
 * @brief Resets standard in/out/err of the current process to defaults.
 *  
 */
void f_dup_reset();

/**
 * @brief Implmentation of 'cat'.
 * 
 */
void f_cat();

/**
 * @brief Implementation of 'ls'.
 * 
 */
void f_ls();

/**
 * @brief Implmentation of 'touch'
 * 
 * @note If a file with name fname already exists, this
 * function does nothing.
 * 
 * @param fname 
 */
void f_touch(char* fname);

/**
 * @brief Implmentation of 'rm'.
 * 
 * @note If no file with name fname exists, this function
 * does nothing.
 * @note If the file with name fname has open file descriptors
 * referring to it, it will not be deleted until said descriptors
 * are closed. (Will still delete when filesystem is freed)
 * 
 * @param fname 
 */
void f_rm(char* fname);

/**
 * @brief Resets file system to new state
 * 
 */
void f_reset();

/**
 * @brief Writes 'size' random chars to S_STDOUT in the range
 * [RC_MIN_CHAR, RC_MIN_CHAR].
 * 
 * @param size expected range: [RC_MIN_SIZE, RC_MAX_SIZE]
 */
void f_randchars(int size);

/**
 * @brief Implmentation of 'perror'
 * @note Uses the global constant in errors.c
 * 
 * @param prefix 
 */
void p_perror(const char* prefix);

#endif