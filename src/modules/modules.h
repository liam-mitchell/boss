#ifndef __MODULES_H_
#define __MODULES_H_

typedef struct multiboot_info multiboot_info_t;

void init_modules(multiboot_info_t *mboot);

#endif // __MODULES_H_
