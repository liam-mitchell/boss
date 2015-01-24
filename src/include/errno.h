#ifndef __ERRNO_H_
#define __ERRNO_H_

int errno;

#define ENOENT 2
#define EIO    6
#define EBADF  9
#define ENOMEM 12
#define EFAULT 14
#define EBUSY  16
#define ENODIR 20
#define EISDIR 21
#define EINVAL 22
#define ENFILE 23
#define ENOSYS 38

char *strerror(int err);
int strerror_r(int err, char *buf, int buflen);

#endif // __ERRNO_H_
