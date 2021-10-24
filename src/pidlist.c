#include <stdio.h> // Deubg
#include <stdlib.h>

#include "pidlist.h"

pid_list *closed_plist;
pid_list *sh_closed_plist;

pid_list* plist_init()
{
    pid_list *plist = malloc(sizeof(pid_list));
    
    plist->next = NULL;
    plist->last = &(plist->next);
    plist->len = 0;

    return plist;
}

void plist_release(pid_list *plist)
{
    pid_node **ptr = &(plist->next);
    pid_node *tmp;

    while ((tmp = *ptr)) {
        *ptr = tmp->next;
        free(tmp);
    }

    free(plist);
}

void plist_insert(pid_list *plist, pid_t pid)
{
    pid_node *new_node = malloc(sizeof(pid_node));

    new_node->next = NULL;
    new_node->pid  = pid;

    *(plist->last) = new_node;
    plist->last = &(new_node->next);

    plist->len += 1;
}

void plist_merge(pid_list *plist1, pid_list *plist2)
{
    *(plist1->last) = plist2->next;
    plist1->len += plist2->len;

    plist2->next = NULL;
    plist2->last = &(plist2->next);
    plist2->len  = 0;
}

void plist_delete_intersect(pid_list *plist1, pid_list *plist2)
{
    // TODO: Optimization
    pid_node **pa;
    pid_node *ta;
    pid_node **pb;
    pid_node *tb;

    pa = &(plist1->next);

    while(plist1->len && plist2->len && (ta = *pa)) {
        int found = 0;

        pb = &(plist2->next);

        while ((tb = *pb)) {
            if (ta->pid == tb->pid) {
                *pa = ta->next;
                *pb = tb->next;
                free(ta);
                free(tb);
                plist1->len -= 1;
                plist2->len -= 1;
                found = 1;
                break;
            }

            pb = &(tb->next);
        }

        if (!found)
            pa = &(ta->next);
    }
}

int plist_delete_by_pid(pid_list *plist1, pid_t pid)
{
    pid_node **pa;
    pid_node *ta;

    if (!plist1->len)
        return 0;

    pa = &(plist1->next);

    while((ta = *pa)) {
        if (ta->pid == pid) {
            *pa = ta->next;
            free(ta);
            plist1->len -= 1;
            return 1;
        }
        pa = &(ta->next);
    }

    return 0;
}
