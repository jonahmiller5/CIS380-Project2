#ifndef _DESCRIPTORS_H
#define _DESCRIPTORS_H

#include "directory.h"

#define SUCCESS 0
#define FAILURE -1

/**
 * @brief The maximum number of allowed file descriptors per
 * process
 */
#define FD_LIST_SIZE    64

/**
 * @brief Smallest available fd number (0,1,2 are taken)
 */
#define MIN_FREE_FD     3

/**
 * @brief This represents all the information corresponding
 * to the file descriptor fd. This includes the mode (e.g. 
 * F_READ | F_WRITE) and the cursor
 * 
 */
typedef struct FD_NODE {
    //FILE_NODE * file;
    char* fname;
    int mode;
    int open_fds;
    unsigned int cursor;
} FD_NODE;

/**
 * @brief This contains a pointer to the head of a list
 * of file descriptors, sorted by fd value
 * 
 */
typedef struct FD_LIST {
    FD_NODE** fds;
} FD_LIST;

/**
 * @brief Initializes a new FD_LIST and returns 
 * a pointer to it
 * 
 * @return FD_LIST* 
 */
//FD_LIST* init_fd_list();
FD_LIST* init_fd_list(FILE_LIST* file_list);

/**
 * @brief Frees the given FD_LIST
 * 
 * Note: this will not free the FILE_NODE
 * referenced by list -> file
 * 
 * @param list 
 */
void free_fd_list(FD_LIST* list);

/**
 * @brief Frees the given FD_NODE
 * 
 * @param node 
 */
void free_fd_node(FD_NODE* node);

/**
 * @brief Get a file descriptor for the specified file.
 * 
 * @note This function will also create a new node with the information
 * and add it to the FD_LIST.
 * @note If there are no more fds available, -1 is returned
 * 
 * //@param list
 * @param file_list
 * @param fd_list
 * @param file
 * @param mode
 * @return int
 */
//int get_new_fd(FD_LIST* list, FILE_NODE* file, int mode);
int get_new_fd(FILE_LIST* file_list, FD_LIST* fd_list, FILE_NODE* file, int mode);

/**
 * @brief Creates a fd node object with the specified information
 * and returns a pointer to it
 * 
 * //@param file
 * @param fname
 * @param mode 
 * @return FD_NODE
 */
//FD_NODE* create_fd_node(FILE_NODE* file, int mode);
FD_NODE* create_fd_node(FILE_LIST* file_list, char* fname, int mode);

/**
 * @brief Inserts the node into the list, sorted by fd
 * 
 * @param fd 
 * @param list 
 * @param node
 */
void set_fd_node(int fd, FD_LIST* list, FD_NODE* node);

/**
 * @brief Remove the FD_NODE with the specified fd. This frees
 * the FD_NODE and sets the pointer to NULL. If the actual FD_NODE
 * is removed (read freed), return TRUE.
 * 
 * @param fd 
 * @param list
 * @return int
 */
int remove_fd_node(int fd, FD_LIST* list);

/**
 * @brief Looks through the list and returns the lowest
 * file descriptor available >= list->min
 * 
 * @param list 
 * @return int 
 */
int find_free_fd(FD_LIST* list);

/**
 * @brief Find a FD_NODE with the given fd.
 * If no match is found, return NULL.
 * 
 * @param fd 
 * @param list
 * @return FD_NODE* 
 */
FD_NODE* get_node_by_fd(int fd, FD_LIST* list);

/**
 * @brief Create a dummy FD_NODE with null file
 * pointer and mode = 0
 * 
 * @param file_list
 * @return FD_NODE* 
 */
//FD_NODE* create_dummy_fd_node();
FD_NODE* create_dummy_fd_node(FILE_LIST* file_list);

#endif