#pragma once

struct List {
    List* prev;
    void* addr;
    List* next;
    List(void* elem) : prev(0), next(0), addr(0) { 
        addr = (List *)malloc(sizeof(List));
        if (addr) {
            prev = next = this;
            addr = elem;
        }
    };
};

struct Stack {
    void* addr;
    Stack* next;
    Stack(void* elem) : next(0), addr(elem) { };
};

struct Tree {
    void* addr;
    Tree* right;
    Tree* left;
    Tree(void* elem) : right(0), left(0), addr(elem) { };
};

template <typename Element> class LinkedList {
private:
    List *list = nullptr;
    int count = 0;
public:
    typedef List* PosPtr;
    LinkedList(Element elem) {
        list = new List((void*)elem);
    };
    LinkedList() {
        make_empty();
    }
    List *get() {
        return list;
    }
    List* make_empty();
    PosPtr find(Element elem);
    PosPtr find_previous(Element elem);
    List* insert(Element elem, PosPtr ppos);
    List* insert(int index, Element element);
    List* first();
    List* last();
    PosPtr advance(PosPtr ppos);
    Element retrieve(PosPtr ppos);
    List* add(Element elem);
    void Delete(Element x);
    bool isEmpty();
    int indexOf(Element elem);
    bool contains(Element elem);
    void remove(Element elem);
    int size();
};

template <typename Element> class ListStack {
private:
    Stack *stack = new Stack(nullptr);
public:
    ListStack(Stack S) {
        stack = &S;
    };
    bool isEmpty(Stack stack);
    Stack create();
    void dispose(Stack stack);
    void make_empty(Stack statck);
    Stack push(Element elem, Stack stack);
    Element top(Stack stack);
    void pop(Stack stack);
};

template <typename Element> class BinaryTree {
private:
    Tree *tree = new Tree(nullptr);
public:
    typedef Tree* PosPtr;
    BinaryTree(Tree T) {
        tree = &T;
    };
    Tree* make_empty();
    PosPtr find(Element elem);
    PosPtr min();
    PosPtr max();
    Tree* insert(Element x);
    Tree* Delete(Element x);
    Element retrieve(PosPtr ppos);
};

template<typename Element>
inline List * LinkedList<Element>::make_empty()
{
    if (list->addr != nullptr) {
        list->addr = nullptr;
    }
    if (list->prev != nullptr) {
        delete list->prev;
        list->prev = nullptr;
    }
    if (list->next != nullptr) {
        delete list->next;
        list->next = nullptr;
    }
    count = 0;
    return list;
}

template<typename Element>
inline List * LinkedList<Element>::find(Element elem)
{
    PosPtr ppos = list;
    while (ppos != nullptr && ppos->addr != elem)
        ppos = ppos->prev;
    if (ppos == nullptr) {
        ppos = list;
        while (ppos != nullptr && ppos->addr != elem)
            ppos = ppos->next;
    }
    return ppos;
}

template<typename Element>
inline List * LinkedList<Element>::find_previous(Element elem)
{
    List* list = find(elem);
    if (list == nullptr)
        return nullptr;
    return list->prev;
}

template<typename Element>
inline List * LinkedList<Element>::insert(Element elem, PosPtr ppos)
{
    List *temp = new List(0);
    temp->prev = list;
    if (ppos != nullptr && nullptr != find((Element)ppos->addr) && list->addr != ppos->addr) {
        temp->next = temp->prev;
        temp->prev = ppos;
    }
    if (temp->addr != nullptr) {
        temp->next->addr = (void*)elem;
    } else {
        temp->addr = (void*)elem;
    }
    list = temp;
    count++;
    return list;
}

template<typename Element>
inline List * LinkedList<Element>::insert(int index, Element element)
{
    List *temp = new List(0);
    List* buff = first();
    int it = 0;
    while (buff != nullptr) {
        if (++it == index) {
            temp->prev = buff->prev;
            temp->next = buff;
            break;
        }
        buff = buff->next;
    }
    temp->addr = (void*)element;
    list = temp;
    count++;
    return list;
}

template<typename Element>
inline List* LinkedList<Element>::first()
{
    List* ppos = list;
    while (ppos->prev != nullptr)
        ppos = ppos->prev;
    return ppos;
}

template<typename Element>
inline List* LinkedList<Element>::last()
{
    List* ppos = list;
    while (ppos->next != nullptr)
        ppos = ppos->next;
    return ppos;
}

template<typename Element>
inline List * LinkedList<Element>::add(Element elem)
{
    List *tmp1 = new List(elem);
    List *tmp2 = list;
    tmp1->prev = tmp2;
    tmp1->next = tmp2->next;
    tmp2->next->prev = tmp1;
    tmp2->next = tmp1;
    count++;
    return list;
}

template<typename Element>
inline bool LinkedList<Element>::isEmpty()
{
    return (list->prev == nullptr && list->addr == nullptr && list->next == nullptr);
}

