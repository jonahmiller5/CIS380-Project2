#ifndef _ERRORS_H
#define _ERRORS_H

/**
 * @brief Maximum size of an error message
 * 
 */
#define ERROR_MSG_SIZE      256

/**
 * @brief The perror format is:
 * PREFIX + ERROR_SEP + MESSAGE
 * 
 */
#define ERROR_SEP               ": "

/**
 * @brief Set errno, using one of the errno
 * constants defined in this file.q
 * 
 * @param errno 
 */
void set_errno(int errno);

/**
 * @brief Get the current value of errno
 * 
 * @return int 
 */
int get_errno();

/* FILESYSTEM */
#define E_INVALID_MODE          1
#define E_FILE_NOT_FOUND        2
#define E_FAT_FULL              3
#define E_INVALID_FNAME         4
#define E_MAX_FDS_OPEN          5
#define E_FD_NOT_FOUND          6
#define E_CANT_WRITE            7
#define E_FILE_IN_USE           8

#define M_INVALID_MODE          "invalid mode: expected [F_READ | F_WRITE | F_APPEND]"
#define M_FILE_NOT_FOUND        "file not found"
#define M_FAT_FULL              "the FAT table is full"
#define M_INVALID_FNAME         "invalid file name: empty or null"
#define M_MAX_FDS_OPEN          "no free file descriptors available"
#define M_FD_NOT_FOUND          "descriptor does not refer to any file"
#define M_CANT_WRITE            "no permission to write"
#define M_FILE_IN_USE           "the file is in use and can't be deleted immediately"

#endif