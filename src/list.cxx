#include <iostream>
#include "slabs.h"

void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

void __list_add(struct list_head *newItem,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = newItem;
	newItem->next = next;
	newItem->prev = prev;
	prev->next = newItem;
}

void list_add(struct list_head *newItem, struct list_head *head)
{
	__list_add(newItem, head, head->next);
}


void list_add_tail(struct list_head *newItem, struct list_head *head)
{
	__list_add(newItem, head->prev, head);
}

void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

void __list_del_entry(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}
#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = (struct list_head* )LIST_POISON1;
	entry->prev = (struct list_head* )LIST_POISON2;
}

void list_del_tail(struct list_head *head)
{
    list_del(head->prev);
}

