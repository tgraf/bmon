/*
 * bmon/list.h		Kernel List Implementation
 *
 * Copied and adapted from the kernel sources
 *
 */

#ifndef BMON_LIST_H_
#define BMON_LIST_H_

#ifdef __APPLE__
/* Apple systems define these macros in system headers, so we undef
 * them prior to inclusion of this file */
#undef LIST_HEAD
#undef LIST_HEAD_INIT
#undef INIT_LIST_HEAD
#endif

struct list_head
{
	struct list_head *	next;
	struct list_head *	prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(struct list_head *obj, struct list_head *prev,
			      struct list_head *next)
{
	prev->next = obj;
	obj->prev = prev;
	next->prev = obj;
	obj->next = next;
}

static inline void list_add_tail(struct list_head *obj, struct list_head *head)
{
	__list_add(obj, head->prev, head);
}

static inline void list_add_head(struct list_head *obj, struct list_head *head)
{
	__list_add(obj, head, head->next);
}

static inline void list_del(struct list_head *obj)
{
	obj->next->prev = obj->prev;
	obj->prev->next = obj->next;
}

static inline int list_empty(struct list_head *head)
{
	return head->next == head;
}

#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - ((size_t) &((type *)0)->member));})

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_at_tail(pos, head, member) \
	((pos)->member.next == (head))

#define list_at_head(pos, head, member) \
	((pos)->member.prev == (head))

#define LIST_SELF(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = { &(name), &(name) }

#define list_first_entry(head, type, member)			\
	list_entry((head)->next, type, member)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &(pos)->member != (head); 	\
	     (pos) = list_entry((pos)->member.next, typeof(*(pos)), member))

#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &(pos)->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define init_list_head(head) \
	do { (head)->next = (head); (head)->prev = (head); } while (0)

#endif
