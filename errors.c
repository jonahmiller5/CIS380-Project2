#include "errors.h"

int errno = 0;

void set_errno(int new_errno)
{
    errno = new_errno;
}

int get_errno()
{
    return errno;
}