#ifndef __LIST_H_
#define __LIST_H_

struct list
{
    struct list *next;
    struct list *prev;
};

void list_init(struct list *list);
void list_insert(struct list *list, struct list *entry);
void list_remove(struct list *entry);
int list_size(struct list *list);

#define LIST_FOR_EACH(list, var) for (var = (list)->next; var != (list); var = var->next)
#define LIST_ENTRY(ptr, struct_name, list_name) (struct_name *)((char *)ptr - offsetof(struct_name, list_name))

#endif
