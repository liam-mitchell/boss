#include "list.h"

void list_init(struct list *list)
{
    list->next = list;
    list->prev = list;
}

void list_insert(struct list *list, struct list *entry)
{
    entry->next = list->next;
    entry->prev = list;

    list->next->prev = entry;
    list->next = entry;
}

void list_remove(struct list *entry)
{
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
}

int list_size(struct list *list)
{
    struct list *entry;
    int n = 0;

    LIST_FOR_EACH(list, entry) {
	++n;
    }

    return n;
}
