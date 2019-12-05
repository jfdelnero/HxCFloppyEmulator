#include "fat_list.h"

//-----------------------------------------------------------------
// fat_list_init:
//-----------------------------------------------------------------
FAT_INLINE void fat_list_init(struct fat_list *list)
{
    FAT_ASSERT(list);

    list->head = list->tail = 0;
}
//-----------------------------------------------------------------
// fat_list_remove:
//-----------------------------------------------------------------
FAT_INLINE void fat_list_remove(struct fat_list *list, struct fat_node *node)
{
    FAT_ASSERT(list);
    FAT_ASSERT(node);

    if(!node->previous)
        list->head = node->next;
    else
        node->previous->next = node->next;

    if(!node->next)
        list->tail = node->previous;
    else
        node->next->previous = node->previous;
}
//-----------------------------------------------------------------
// fat_list_insert_after:
//-----------------------------------------------------------------
FAT_INLINE void fat_list_insert_after(struct fat_list *list, struct fat_node *node, struct fat_node *new_node)
{
    FAT_ASSERT(list);
    FAT_ASSERT(node);
    FAT_ASSERT(new_node);

    new_node->previous = node;
    new_node->next = node->next;
    if (!node->next)
        list->tail = new_node;
    else
        node->next->previous = new_node;
    node->next = new_node;
}
//-----------------------------------------------------------------
// fat_list_insert_before:
//-----------------------------------------------------------------
FAT_INLINE void fat_list_insert_before(struct fat_list *list, struct fat_node *node, struct fat_node *new_node)
{
    FAT_ASSERT(list);
    FAT_ASSERT(node);
    FAT_ASSERT(new_node);

    new_node->previous = node->previous;
    new_node->next = node;
    if (!node->previous)
        list->head = new_node;
    else
        node->previous->next = new_node;
    node->previous = new_node;
}
//-----------------------------------------------------------------
// fat_list_insert_first:
//-----------------------------------------------------------------
FAT_INLINE void fat_list_insert_first(struct fat_list *list, struct fat_node *node)
{
    FAT_ASSERT(list);
    FAT_ASSERT(node);

    if (!list->head)
    {
        list->head = node;
        list->tail = node;
        node->previous = 0;
        node->next = 0;
    }
    else
        fat_list_insert_before(list, list->head, node);
}
//-----------------------------------------------------------------
// fat_list_insert_last:
//-----------------------------------------------------------------
FAT_INLINE void fat_list_insert_last(struct fat_list *list, struct fat_node *node)
{
    FAT_ASSERT(list);
    FAT_ASSERT(node);

    if (!list->tail)
        fat_list_insert_first(list, node);
     else
        fat_list_insert_after(list, list->tail, node);
}

//-----------------------------------------------------------------
// fat_list_is_empty:
//-----------------------------------------------------------------
FAT_INLINE int fat_list_is_empty(struct fat_list *list)
{
    FAT_ASSERT(list);

    return !list->head;
}

//-----------------------------------------------------------------
// fat_list_pop_head:
//-----------------------------------------------------------------
FAT_INLINE struct fat_node * fat_list_pop_head(struct fat_list *list)
{
    struct fat_node * node;

    FAT_ASSERT(list);

    node = fat_list_first(list);
    if (node)
        fat_list_remove(list, node);

    return node;
}
