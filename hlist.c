/***************************************/
Why use **pprev instead of keeping *prev		
/***************************************/		
		1.hlist_head is different from hlist_node, unlike in normal linked list, they are the same list_head
		2.if we use *prev as in list_head, we the implementation of add/delete elements would be more complicated than normal linked list.
		void
		hlist_add_before(struct hlist_head *head,
		                 struct hlist_node *node,
		                 struct hlist_next *next) {
		  hlist_node *prev = next->prev;
		
		  node->next = next;
		  node->prev = prev;
		  next->prev = node;
		
		  if (prev) {
		    prev->next = node;
		  } else {
		    head->first = node;
		  }}
		as showed above, we had to pass *head everytime, because hlist_head is different from hlist_node, 
		need to consider special case that we add a element to the head(*prev = null, *next is the previous head node)
		3.with **pprev it is much easier
		static inline void hlist_add_before(struct hlist_node *n,
		                    struct hlist_node *next)
		{
		    n->pprev = next->pprev;
		    n->next = next;
		    next->pprev = &n->next;
		    *(n->pprev) = n;
		}
		even if *next is the head node, it is the same with other cases.

/***************************************/
rcu version of linked list function
/***************************************/	
static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void __list_add_rcu(struct list_head *new,
		struct list_head *prev, struct list_head *next)
{
	new->next = next;
	new->prev = prev;
	rcu_assign_pointer(list_next_rcu(prev), new);
	next->prev = new;
}

#define __rcu_assign_pointer(p, v, space) \
	({ \
		smp_wmb(); \
		(p) = (typeof(*v) __force space *)(v); \
	})

the difference here is the smp_wmb() and the position of it. This allows us to publish the changes to the list as well as maintain the old structure