#include "test.h"

#include "list.h"

struct list_test
{
    int i;
    struct list l;
};

void test_empty_list(void)
{
    struct list list;
    list_init(&list);

    KASSERT(list_size(&list) == 0);
}

void test_single_element_list(void)
{
    struct list_test s;
    s.i = 42;

    struct list list;
    list_init(&list);
    list_insert(&list, &s.l);

    KASSERT(list_size(&list) == 1);

    struct list *elem;
    LIST_FOR_EACH(&list, elem) {
	struct list_test *entry = LIST_ENTRY(elem, struct list_test, l);
	KASSERT(entry == &s);
	KASSERT(entry->i == 42);
    }

    list_remove(&s.l);

    KASSERT(list_size(&list) == 0);
}

void test_multiple_element_list(void)
{
    struct list_test s[2];
    int expected[2] = {42, 69};

    s[0].i = expected[0];
    s[1].i = expected[1];

    struct list list;
    list_init(&list);

    // List inserts at the head, so insert these backwards to match s[i] == expected[i]
    list_insert(&list, &s[1].l);
    list_insert(&list, &s[0].l);

    KASSERT(list_size(&list) == 2);

    int i = 0;
    struct list *elem;

    LIST_FOR_EACH(&list, elem) {
	struct list_test *entry = LIST_ENTRY(elem, struct list_test, l);
	
	KASSERT(entry->i == expected[i]);
	++i;
    }

    list_remove(&s[0].l);

    KASSERT(list_size(&list) == 1);

    LIST_FOR_EACH(&list, elem) {
	struct list_test *entry = LIST_ENTRY(elem, struct list_test, l);

	KASSERT(entry->i == 69);
    }

    list_remove(&s[1].l);

    KASSERT(list_size(&list) == 0);
}

void test_list(void)
{
    test_empty_list();
    test_single_element_list();
    test_multiple_element_list();
}

void ktest(void)
{
    test_list();
}
