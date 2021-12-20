#include <stdio.h>
#include <stdlib.h>

#include "descriptors.h"

FD_LIST* init_fd_list(FILE_LIST* file_list)
{
    FD_LIST* list = malloc(sizeof(FD_LIST));
    list -> fds = malloc(sizeof(FD_NODE*) * FD_LIST_SIZE);
    
    // printf("list: %d\n", list);
    // printf("list -> fds: %d\n", list -> fds);
    
    for (int i = 0; i < FD_LIST_SIZE; i++) {
        //printf("fd node: %d\n", i);    
        (list -> fds)[i] = i < MIN_FREE_FD ? 
                create_dummy_fd_node(file_list) : NULL;
    }
    return list;
}

void free_fd_list(FD_LIST* list)
{
    for (int i = 0; i < FD_LIST_SIZE; i++) {
        FD_NODE* curr = (list -> fds)[i];
        if (curr != NULL) { free_fd_node(curr); }
    }
    free(list -> fds);
    free(list);
}

void free_fd_node(FD_NODE* node)
{
    free(node);
}

int get_new_fd(FILE_LIST* file_list, FD_LIST* fd_list, FILE_NODE* file, int mode)
{
    int new_fd = find_free_fd(fd_list);
    // printf("  new_fd: %d\n", new_fd);
    if (new_fd == -1) { return -1; }
    FD_NODE* new_node = create_fd_node(file_list, file -> name, mode);
    set_fd_node(new_fd, fd_list, new_node);
    return new_fd;
}

//FD_NODE* create_fd_node(FILE_NODE* file, int mode)
FD_NODE* create_fd_node(FILE_LIST* file_list, char* fname, int mode)
{
    FD_NODE* new_node = malloc(sizeof(FD_NODE));
    //new_node -> file = file;
    new_node -> fname = fname;
    new_node -> mode = mode;
    new_node -> cursor = 0;
    new_node -> open_fds = 0;
    //if (file != NULL) { file -> open_fds++; }
    FILE_NODE* file = find_node_with_name(file_list, fname);
    if (file != NULL) { file -> open_fds++; }
    return new_node;
}

void set_fd_node(int fd, FD_LIST* list, FD_NODE* node)
{
    (list -> fds)[fd] = node;
}

int remove_fd_node(int fd, FD_LIST* list)
{
    // printf("  remove_fd_node: %d\n", fd);
    FD_NODE* node = (list -> fds)[fd];
    if (node == NULL) { return FALSE; }
    set_fd_node(fd, list, NULL);
    if (--(node -> open_fds) <= 0) { 
        free_fd_node(node);
        return TRUE;
    }
    return FALSE;
}

int find_free_fd(FD_LIST* list)
{
    for (int i = MIN_FREE_FD; i < FD_LIST_SIZE; i++)
        if ((list -> fds)[i] == NULL)
            return i;
    return -1;
}

FD_NODE* get_node_by_fd(int fd, FD_LIST* list)
{
    // printf("________________big line guy________________\n");
    // if (list == NULL) {
    //     write_to_terminal("list is NULL\n");
    // }
    // write_to_terminal("About to check list->fds\n");
    // if (list->fds == NULL) {
    //     write_to_terminal("list->fds is NULL\n");
    // }
    // write_to_terminal("About to check (list->fds)[0]\n");
    // if ((list->fds)[0] == NULL) {
    //     write_to_terminal("(list->fds)[0] is NULL\n");
    // }
    // for (int i = 0; i <64; i ++){
    //     if ((list->fds) [i] == NULL){
    //         printf("NULL\n");
    //     } else {
    //         printf("NOT NULL\n");
    //     }
    // }
    return (list -> fds)[fd];
}

FD_NODE* create_dummy_fd_node(FILE_LIST* file_list) {
    return create_fd_node(file_list, NULL, 0);
}