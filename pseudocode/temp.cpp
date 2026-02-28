#include <utility>
template <typename A, typename B>
extern int CAS(A a, B b, B c);

class Node {
    public:
    int val;
    Node* next;
    explicit Node(int i) : val(i) {}
    explicit Node(int i, Node* next) : val(i), next(next) {}
};

class LinkedList {
    public:
    Node* head;
    // initialise sentinel head node (value doesn't matter)
    LinkedList() : head(Node(0)){}

    bool lookup(int i){
        Node* node = head->next;
        while (node && node->val < i){
            node = node->next;
        }
        return (node && node->val == i);
    }

    bool insert(int i){
        Node *node, *next;
        Node* new_node = new Node(i);
        bool success;
        // We assume a sentinel head that is not part of the list we keep track of.
        do {
            node = read(head);
            next = node->next;
            success = false;
            while (next && next->val < i){
                node = next;
                next = node->next;
            }
            if (next && next->val == i){
                return false;
            }
            new_node->next = next;
            success = CAS(&node->next, next, new_node);
        } while (!success);
    }
};
