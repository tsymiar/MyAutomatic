#pragma once

#include <malloc.h>

struct List {
    List* prev;
    void* addr;
    List* next;
    List(void* elem) : prev(0), next(0), addr(0) {
        addr = (List*)malloc(sizeof(List));
        if (addr != NULL) {
            prev = next = this;
            addr = elem;
        }
    };
    ~List() {
        if (addr != NULL)
            free(addr);
    }
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
    List* list = nullptr;
    int count = 0;
public:
    typedef List* PosPtr;
    LinkedList(Element elem) {
        list = new List((void*)elem);
        count++;
    };
    ~LinkedList() {
        if (makeEmpty() != nullptr)
            delete list;
    }
    List* get() {
        return list;
    }
    PosPtr find(Element elem);
    PosPtr find_previous(Element elem);
    List* insert(Element elem, PosPtr ppos);
    List* insert(int index, Element element);
    List* add(Element elem);
    List* get(int num);
    int size();
    void remove(Element elem);
    bool contains(Element elem);
    PosPtr advance(PosPtr ppos, int step);
    Element retrieve(PosPtr ppos);
    List* first();
    List* last();
    List* makeEmpty();
    int indexOf(Element elem);
    bool isEmpty();
    void Delete(Element elem);
};

template <typename Element> class ListStack {
private:
    Stack* stack = new Stack(nullptr);
public:
    ListStack(Element S) {
        stack->addr = (void*)S;
    };
    ~ListStack() {
        dispose();
    };
    bool is_empty();
    void dispose();
    void make_empty();
    Stack* push(Element elem);
    Element top();
    void pop();
};

template <typename Element> class BinaryTree {
private:
    Tree* tree = new Tree(nullptr);
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
inline List* LinkedList<Element>::find(Element elem)
{
    PosPtr ppos = list;
    int i = 0;
    while (ppos != nullptr && ppos->addr != elem) {
        if (++i > count) {
            ppos = nullptr;
            break;
        }
        ppos = ppos->prev;
    }
    if (ppos == nullptr) {
        i = 0;
        ppos = list;
        while (ppos != nullptr && ppos->addr != elem) {
            if (++i > count) {
                ppos = nullptr;
                break;
            }
            ppos = ppos->next;
        }
    }
    return ppos;
}

template<typename Element>
inline List* LinkedList<Element>::find_previous(Element elem)
{
    List* ppos = find(elem);
    if (ppos == nullptr)
        return nullptr;
    return ppos->prev;
}

template<typename Element>
inline List* LinkedList<Element>::insert(Element elem, PosPtr ppos)
{
    List* temp = new List(0);
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
inline List* LinkedList<Element>::insert(int index, Element element)
{
    if (index <= 0 || index > count) {
        return nullptr;
    }
    List* elem = new List((void*)element);
    List* buff = first();
    int it = 0;
    while (buff != nullptr) {
        if (++it == index) {
            elem->prev = buff;
            elem->next = buff->next;
            buff->next->prev = elem;
            buff->next = elem;
            break;
        }
        buff = buff->next;
    }
    count++;
    return list;
}

template<typename Element>
inline List* LinkedList<Element>::add(Element elem)
{
    List* tmp1 = new List((void*)elem);
    List* tmp2 = list->prev;
    tmp1->prev = tmp2;
    tmp1->next = tmp2->next;
    tmp2->next->prev = tmp1;
    tmp2->next = tmp1;
    count++;
    return list;
}

template<typename Element>
inline List* LinkedList<Element>::get(int num)
{
    if (num < 0 || num >= count)
        return nullptr;
    int i = 0;
    List* ppos = list;
    while (ppos->next != nullptr) {
        if (num == 0)
            return ppos;
        if (i == num)
            break;
        ppos = ppos->next;
        i++;
    }
    return ppos;
}

template<typename Element>
inline int LinkedList<Element>::size()
{
    return count;
}

template<typename Element>
inline void LinkedList<Element>::remove(Element elem)
{
    List* ppos = find(elem);
    if (ppos != nullptr) {
        ppos->prev->next = ppos->next;
        ppos->next->prev = ppos->prev;
        delete ppos;
        count--;
    }
}

template<typename Element>
inline bool LinkedList<Element>::contains(Element elem)
{
    return (find(elem) != nullptr);
}

template<typename Element>
inline List* LinkedList<Element>::advance(PosPtr ppos, int step)
{
    if (step < 0)
        return nullptr;
    for (int i = 0; i < step; i++)
        ppos = ppos->next;
    return ppos;
}

template<typename Element>
inline Element LinkedList<Element>::retrieve(PosPtr ppos)
{
    return ppos->addr;
}

template<typename Element>
inline List* LinkedList<Element>::first()
{
    int i = count;
    List* ppos = list;
    while (ppos->prev != nullptr) {
        if (--i < 0)
            break;
        ppos = ppos->prev;
    }
    return ppos;
}

template<typename Element>
inline List* LinkedList<Element>::last()
{
    return get(count - 1);
}

template<typename Element>
inline List* LinkedList<Element>::makeEmpty()
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
inline int LinkedList<Element>::indexOf(Element elem)
{
    List* ppos = list;
    for (int i = 0; i < count; i++) {
        if (ppos->addr == elem)
            return i;
        ppos = ppos->next;
    }
    return -1;
}

template<typename Element>
inline bool LinkedList<Element>::isEmpty()
{
    return (list->prev == nullptr && list->addr == nullptr && list->next == nullptr);
}

template<typename Element>
inline void LinkedList<Element>::Delete(Element elem)
{
    remove(elem);
}

template<typename Element>
inline bool ListStack<Element>::is_empty()
{
    return (stack == nullptr || (stack->addr == nullptr && stack->next == nullptr));
}

template<typename Element>
inline void ListStack<Element>::dispose()
{
    if (stack != nullptr) {
        if (stack->next != nullptr)
            delete stack->next;
        delete stack;
        stack = nullptr;
    }
}

template<typename Element>
inline void ListStack<Element>::make_empty()
{
    while (!is_empty()) {
        pop();
    }
}

template<typename Element>
inline Stack* ListStack<Element>::push(Element elem)
{
    Stack* ppos = new Stack((void*)elem);
    ppos->next = stack;
    stack = ppos;
    return stack;
}

template<typename Element>
inline Element ListStack<Element>::top()
{
    return Element(stack->addr);
}

template<typename Element>
inline void ListStack<Element>::pop()
{
    Stack* ppos = stack->next;
    delete stack;
    stack = ppos;
}
