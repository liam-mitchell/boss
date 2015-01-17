#include "errno.h"

#include "string.h"

char *strerror(int err)
{
    static char buf[80] = { 0 };

    switch (err) {
    case ENOENT:
        strncpy(buf, "Operation not permitted", 80);
        break;
    case EIO:
        strncpy(buf, "No such device or address", 80);
        break;
    case EBADF:
        strncpy(buf, "Bad file descriptor", 80);
        break;
    case ENOMEM:
        strncpy(buf, "Cannot allocate memory", 80);
        break;
    case EFAULT:
        strncpy(buf, "Bad address", 80);
        break;
    case ENODIR:
        strncpy(buf, "Not a directory", 80);
        break;
    case EISDIR:
        strncpy(buf, "Is a directory", 80);
        break;
    case EINVAL:
        strncpy(buf, "Invalid argument", 80);
        break;
    case ENOSYS:
        strncpy(buf, "Function not implemented", 80);
        break;
    default:
        strncpy(buf, "Unknown error", 80);
        break;
    }

    return buf;
}
