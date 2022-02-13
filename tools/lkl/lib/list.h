
struct list_node{
    struct list_node *next, *prev;
};

#define LIST_ADD(head, node) \
    list_add((struct list_node**) &head, (struct list_node*)node)

static void list_add(struct list_node **head, struct list_node *node) {
    if (*head)
        (*head)->prev = node;
    node->next = *head;
    *head = node;
}
#define LIST_DEL(head, node) \
    list_del((struct list_node**) &head, (struct list_node*)node)
static void list_del(struct list_node **head, struct list_node *node) {
    if (node->prev) {
        node->prev->next = node->next;
    }
    else {
        *head = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    }
}

#define list_for_each(ptr, head) \
    for (ptr = head; ptr != NULL; ptr = ptr->next)
