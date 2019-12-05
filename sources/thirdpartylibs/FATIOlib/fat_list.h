#ifndef __FAT_LIST_H__
#define __FAT_LIST_H__

#ifndef FAT_ASSERT
    #define FAT_ASSERT(x)
#endif

#ifndef FAT_INLINE
    #define FAT_INLINE
#endif

//-----------------------------------------------------------------
// Types
//-----------------------------------------------------------------
struct fat_list;

struct fat_node
{
    struct fat_node    *previous;
    struct fat_node    *next;
};

struct fat_list
{
    struct fat_node    *head;
    struct fat_node    *tail;
};

//-----------------------------------------------------------------
// Macros
//-----------------------------------------------------------------
#define fat_list_entry(p, t, m)     p ? ((t *)((char *)(p)-(char*)(&((t *)0)->m))) : 0
#define fat_list_next(l, p)         (p)->next
#define fat_list_prev(l, p)         (p)->previous
#define fat_list_first(l)           (l)->head
#define fat_list_last(l)            (l)->tail
#define fat_list_for_each(l, p)     for ((p) = (l)->head; (p); (p) = (p)->next)

//-----------------------------------------------------------------
// Inline Functions
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// fat_list_init:
//-----------------------------------------------------------------
FAT_INLINE void fat_list_init(struct fat_list *list);

//-----------------------------------------------------------------
// fat_list_remove:
//-----------------------------------------------------------------
FAT_INLINE void fat_list_remove(struct fat_list *list, struct fat_node *node);

//-----------------------------------------------------------------
// fat_list_insert_after:
//-----------------------------------------------------------------
FAT_INLINE void fat_list_insert_after(struct fat_list *list, struct fat_node *node, struct fat_node *new_node);

//-----------------------------------------------------------------
// fat_list_insert_before:
//-----------------------------------------------------------------
FAT_INLINE void fat_list_insert_before(struct fat_list *list, struct fat_node *node, struct fat_node *new_node);

//-----------------------------------------------------------------
// fat_list_insert_first:
//-----------------------------------------------------------------
FAT_INLINE void fat_list_insert_first(struct fat_list *list, struct fat_node *node);

//-----------------------------------------------------------------
// fat_list_insert_last:
//-----------------------------------------------------------------
FAT_INLINE void fat_list_insert_last(struct fat_list *list, struct fat_node *node);

//-----------------------------------------------------------------
// fat_list_is_empty:
//-----------------------------------------------------------------
FAT_INLINE int fat_list_is_empty(struct fat_list *list);

//-----------------------------------------------------------------
// fat_list_pop_head:
//-----------------------------------------------------------------
FAT_INLINE struct fat_node * fat_list_pop_head(struct fat_list *list);

#endif
