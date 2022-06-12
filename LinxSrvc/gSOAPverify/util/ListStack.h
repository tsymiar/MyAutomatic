#pragma once

#include <cstdlib>

struct List {
    char check;
    void* addr;
    List* prev;
    List* next;
    List() : prev(NULL),
        next(NULL),
        addr(NULL), check(0) {
        addr = malloc(sizeof(List));
    }
    ~List() {
        if (addr != NULL && !check) {
            free(addr);
            addr = NULL;
        }
    }
    List(void* elem) : check(1) {
        prev = next = NULL;
        if (elem != NULL) {
            addr = elem;
        } else {
            addr = NULL;
        }
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
    List* insert(Element elem, PosPtr posp);
    List* insert(int index, Element element);
    List* add(List* node, PosPtr posp);
    List* add(Element elem);
    List* get(int num);
    int size();
    void remove(Element elem);
    bool contains(Element elem);
    PosPtr advance(PosPtr posp, int step);
    Element retrieve(PosPtr posp);
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
    void MakeEmpty(PosPtr posp);
    Tree* Find(Element elem, PosPtr posp);
    Tree* Min(PosPtr posp);
    Tree* Max(PosPtr posp);
    Tree* Insert(Element elem, PosPtr posp);
    Tree* Delete(Element elem, PosPtr posp);
    Element Retrieve(PosPtr posp);
};

template<typename Element>
inline List* LinkedList<Element>::find(Element elem)
{
    PosPtr posp = list;
    int i = 0;
    while (posp != nullptr && posp->addr != elem) {
        if (++i > count) {
            posp = nullptr;
            break;
        }
        posp = posp->prev;
    }
    if (posp == nullptr) {
        i = 0;
        posp = list;
        while (posp != nullptr && posp->addr != elem) {
            if (++i > count) {
                posp = nullptr;
                break;
            }
            posp = posp->next;
        }
    }
    return posp;
}

template<typename Element>
inline List* LinkedList<Element>::find_previous(Element elem)
{
    List* posp = this->find(elem);
    if (posp == nullptr)
        return nullptr;
    return posp->prev;
}

template<typename Element>
inline List* LinkedList<Element>::insert(Element elem, PosPtr posp)
{
    List* temp = new List();
    temp->prev = list;
    if (posp != nullptr && nullptr != find((Element)posp->addr) && list->addr != posp->addr) {
        temp->next = temp->prev;
        temp->prev = posp;
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
            buff = this->add(elem, buff);
            break;
        }
        buff = buff->next;
    }
    return list;
}

template<typename Element>
inline List* LinkedList<Element>::add(List* node, PosPtr posp)
{
    if (posp->next == nullptr) {
        node->prev = posp;
        posp->next = node;
    } else if (posp->prev == nullptr) {
        node->next = posp;
        posp->prev = node;
    } else {
        node->prev = posp;
        node->next = posp->next;
        posp->next->prev = node;
        posp->next = node;
    }
    count++;
    return posp;
}

template<typename Element>
inline List* LinkedList<Element>::add(Element elem)
{
    List* node = new List((void*)elem);
    return this->add(node, list);
}

template<typename Element>
inline List* LinkedList<Element>::get(int num)
{
    if (num < 0 || num >= count)
        return nullptr;
    int i = 0;
    List* posp = list;
    while (posp->next != nullptr) {
        if (num == 0)
            return posp;
        if (i == num)
            break;
        posp = posp->next;
        i++;
    }
    return posp;
}

template<typename Element>
inline int LinkedList<Element>::size()
{
    return count;
}

template<typename Element>
inline void LinkedList<Element>::remove(Element elem)
{
    List* posp = find(elem);
    if (posp != nullptr) {
        posp->prev->next = posp->next;
        posp->next->prev = posp->prev;
        delete posp;
        count--;
    }
}

template<typename Element>
inline bool LinkedList<Element>::contains(Element elem)
{
    return (find(elem) != nullptr);
}

template<typename Element>
inline List* LinkedList<Element>::advance(PosPtr posp, int step)
{
    if (step < 0)
        return nullptr;
    for (int i = 0; i < step; i++)
        posp = posp->next;
    return posp;
}

template<typename Element>
inline Element LinkedList<Element>::retrieve(PosPtr posp)
{
    return posp->addr;
}

template<typename Element>
inline List* LinkedList<Element>::first()
{
    int i = count;
    List* posp = list;
    while (posp->prev != nullptr) {
        if (--i < 0)
            break;
        posp = posp->prev;
    }
    return posp;
}

template<typename Element>
inline List* LinkedList<Element>::last()
{
    return this->get(count - 1);
}

template<typename Element>
inline List* LinkedList<Element>::makeEmpty()
{
    if (list->prev != nullptr) {
        delete list->prev;
        list->prev = nullptr;
    }
    if (list->next != nullptr) {
        delete list->next;
        list->next = nullptr;
    }
    if (list->addr != nullptr) {
        list->addr = nullptr;
    }
    count = 0;
    return list;
}

template<typename Element>
inline int LinkedList<Element>::indexOf(Element elem)
{
    List* posp = list;
    for (int i = 0; i < count; i++) {
        if (posp->addr == elem)
            return i;
        posp = posp->next;
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
    this->remove(elem);
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
        this->pop();
    }
}

template<typename Element>
inline Stack* ListStack<Element>::push(Element elem)
{
    Stack* posp = new Stack((void*)elem);
    posp->next = stack;
    stack = posp;
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
    Stack* posp = stack->next;
    delete stack;
    stack = posp;
}

template<typename Element>
inline void BinaryTree<Element>::MakeEmpty(PosPtr posp)
{
    if (posp != nullptr) {
        this->make_empty(posp->left);
        this->make_empty(posp->right);
        delete posp;
    }
}

template<typename Element>
inline Tree* BinaryTree<Element>::Find(Element elem, PosPtr posp)
{
    if (posp == nullptr) {
        return nullptr;
    }
    if (elem < posp->addr) {
        return this->find(elem, posp->left);
    } else if (elem > posp->addr) {
        return this->find(elem, posp->right);
    } else {
        return posp;
    }
}

template<typename Element>
inline Tree* BinaryTree<Element>::Min(PosPtr posp)
{
    if (posp == nullptr) {
        return nullptr;
    } else if (posp->left == nullptr) {
        return posp;
    } else {
        return min(posp->left);
    }
}

template<typename Element>
inline Tree* BinaryTree<Element>::Max(PosPtr posp)
{
    if (posp != nullptr) {
        while (posp->right != nullptr) {
            posp = posp->right;
        }
    }
    return posp;
}

template<typename Element>
inline Tree* BinaryTree<Element>::Insert(Element elem, PosPtr posp)
{
    if (posp == nullptr) {
        posp = new Tree();
        if (posp != nullptr) {
            posp->addr = elem;
            posp->left = posp->right = nullptr;
        }
    } else if (elem < posp->addr) {
        posp->left = Insert(elem, posp->left);
    } else if (elem > posp->addr) {
        posp->right = Insert(elem, posp->right);
    }
    return posp;
}

template<typename Element>
inline Tree* BinaryTree<Element>::Delete(Element elem, PosPtr posp)
{
    PosPtr cell;
    if (posp != nullptr) {
        if (elem < posp->addr) {
            posp->left = Delete(elem, posp->left);
        }
        if (elem > posp->addr) {
            posp->right = Delete(elem, posp->left);
        }
        if (posp->left && posp->right) {
            cell = Min(posp->right);
            posp->addr = cell->addr;
            posp->right = Delete(posp->addr, posp->right);
        } else {
            cell = posp;
            if (posp->left == nullptr) {
                posp = posp->right;
            } else if (posp->right == nullptr) {
                posp = posp->left;
            }
            delete cell;
        }
    }
    return posp;
}

template<typename Element>
inline Element BinaryTree<Element>::Retrieve(PosPtr posp)
{
    return posp->addr;
}
