#ifndef PIDLIST_H
#define PIDLIST_H

#include <unistd.h>

typedef struct pid_node_tag pid_node;
struct pid_node_tag {
    pid_node *next;
    pid_t pid;
};

typedef struct pid_node_list_tag pid_list;
struct pid_node_list_tag {
    pid_node *next;
    pid_node **last;
    int len;
};

extern pid_list *closed_plist;
extern pid_list *sh_closed_plist;

extern pid_list* plist_init();

extern void plist_release(pid_list *plist);

extern void plist_insert(pid_list *plist, pid_t pid);

// Block signal handler
extern void plist_insert_block(pid_list *plist, pid_t pid);

// Link plist2 node to plist1, and leave plist2 empty.
extern void plist_merge(pid_list *plist1, pid_list *plist2);

// When two nodes with the same pid exist in both plist1 and plist2
// Delete these nodes
extern void plist_delete_intersect(pid_list *plist1, pid_list *plist2);

// Block signal handler
extern void plist_delete_intersect_block(pid_list *plist1, pid_list *plist2);

extern int plist_delete_by_pid(pid_list *plist1, pid_t pid);

#endif