#pragma once

#include <stdlib.h>
#include <stdio.h>

#ifndef WIN32
#ifndef nullptr
#define nullptr NULL
#endif
#endif // !WIN32

struct List {
    bool check;
    void* addr;
    List* prev;
    List* next;
    List() : check(0),
        addr(NULL), prev(NULL), next(NULL) {
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
    Stack(void* elem) : addr(elem), next(0) { };
};

struct Tree {
    void* addr;
    Tree* right;
    Tree* left;
    int height;
    Tree(void* elem) : addr(elem), right(0), left(0) {
        height = 0;
    };
    Tree() : addr(0), right(0), left(0) {
        height = 0;
    };
};

struct Hash {
    int size;
    List* lists;
};

template <typename Element> class LinkedList {
private:
    List* m_list;
    int m_count;
public:
    typedef List* PosPtr;
    LinkedList(Element elem) : m_list(nullptr), m_count(0) {
        m_list = new List((void*)elem);
        m_count++;
    }
    ~LinkedList() {
        if (this->MakeEmpty() != nullptr)
            delete m_list;
    }
    List* get() {
        return m_list;
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
    List* MakeEmpty();
    int indexOf(Element elem);
    bool isEmpty();
    void Delete(Element elem);
};

template <typename Element> class ListStack {
private:
    Stack* m_stack;
public:
    ListStack(Element S) {
        m_stack = new Stack(S);
    }
    ~ListStack() {
        dispose();
    }
    bool is_empty();
    void dispose();
    void make_empty();
    Stack* push(Element elem);
    Element top();
    void pop();
};

template <typename Element> class BinaryTree {
private:
    Tree* m_btree;
public:
    typedef Tree* PosPtr;
    BinaryTree(Element T) {
        m_btree = new Tree(T);
    }
    virtual ~BinaryTree() {
        if (this->MakeEmpty(m_btree) != nullptr) {
            delete m_btree;
            m_btree = nullptr;
        }
    }
public:
    PosPtr MakeEmpty(PosPtr posp);
    PosPtr Find(Element elem, PosPtr posp);
    PosPtr Min(PosPtr posp);
    PosPtr Max(PosPtr posp);
    virtual PosPtr Insert(Element elem, PosPtr& posp);
    PosPtr Delete(Element elem, PosPtr& posp);
    Element Retrieve(PosPtr posp);
    void PrintTree(PosPtr posp);
};

template <typename Element> class AVLBinTree : public BinaryTree<Element> {
public:
    AVLBinTree(Element T) : BinaryTree<Element>(T) { }
    using AvlTree = Tree*;
    static int max(int x, int y) {
        return x > y ? x : y;
    }
    static int Height(AvlTree posp) {
        if (posp == nullptr)
            return -1;
        else
            return posp->height;
    }
    static AvlTree SingleRotateWithLeft(AvlTree k2)
    {
        AvlTree k1 = nullptr;
        if (k2 != nullptr)
            k1 = k2->left;
        if (k1 != nullptr && k2 != nullptr)
            k2->left = k1->right;
        if (k1 != nullptr)
            k1->right = k2;
        if (k2 != nullptr)
            k2->height = max(Height(k2->left), Height(k2->right)) + 1;
        if (k1 != nullptr && k2 != nullptr)
            k1->height = max(Height(k1->left), k2->height) + 1;
        return k1;
    }
    static AvlTree DoubleRotateWithLeft(AvlTree k3)
    {
        if (k3 != nullptr && k3->left != nullptr)
            k3->left = SingleRotateWithLeft(k3->left);
        return SingleRotateWithLeft(k3);
    }
    AvlTree Insert(Element elem, AvlTree& posp) override;
    int HeightBt(AvlTree posp) {
        if (posp == nullptr)
            return -1;
        else
            return (1 + max(HeightBt(posp->left), HeightBt(posp->right)));
    }
};

template <typename Element> class HashTable {
private:
    int m_minSize = 1;
public:
    typedef unsigned int Index;
    using HashTbl = Hash*;
    Index MakeHash(const char* key, int size);
    HashTbl Initialize(int size);
    void Destroy(HashTbl tbl);
    HashTbl Find(Element elem, HashTbl tbl);
    HashTbl Insert(Element elem, HashTbl tbl);
    Element Retrieve(HashTbl tbl);
};

template<typename Element>
inline List* LinkedList<Element>::find(Element elem)
{
    PosPtr posp = m_list;
    int i = 0;
    while (posp != nullptr && posp->addr != elem) {
        if (++i > m_count) {
            posp = nullptr;
            break;
        }
        posp = posp->prev;
    }
    if (posp == nullptr) {
        i = 0;
        posp = m_list;
        while (posp != nullptr && posp->addr != elem) {
            if (++i > m_count) {
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
    List* list = new List();
    list->prev = m_list;
    if (posp != nullptr && nullptr != find((Element)posp->addr) &&
        m_list->addr != posp->addr) {
        list->next = list->prev;
        list->prev = posp;
    }
    if (list->addr != nullptr) {
        if (list->next == nullptr) {
            list->next = new List((void*)elem);
        } else {
            list->next->addr = (void*)elem;
        }
    } else {
        list->addr = (void*)elem;
    }
    m_list = list;
    m_count++;
    return m_list;
}

template<typename Element>
inline List* LinkedList<Element>::insert(int index, Element element)
{
    if (index <= 0 || index > m_count) {
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
    return m_list;
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
    m_count++;
    return posp;
}

template<typename Element>
inline List* LinkedList<Element>::add(Element elem)
{
    List* node = new List((void*)elem);
    return this->add(node, m_list);
}

template<typename Element>
inline List* LinkedList<Element>::get(int num)
{
    if (num < 0 || num >= m_count)
        return nullptr;
    int i = 0;
    List* posp = m_list;
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
    return m_count;
}

template<typename Element>
inline void LinkedList<Element>::remove(Element elem)
{
    List* posp = find(elem);
    if (posp != nullptr) {
        if (posp->prev != nullptr)
            posp->prev->next = posp->next;
        posp->next->prev = posp->prev;
        delete posp;
        m_count--;
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
        if (posp != nullptr)
            posp = posp->next;
        else
            break;
    return posp;
}

template<typename Element>
inline Element LinkedList<Element>::retrieve(PosPtr posp)
{
    return static_cast<Element>(posp->addr);
}

template<typename Element>
inline List* LinkedList<Element>::first()
{
    int i = m_count;
    List* posp = m_list;
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
    return this->get(m_count - 1);
}

template<typename Element>
inline List* LinkedList<Element>::MakeEmpty()
{
    if (m_list->prev != nullptr) {
        delete m_list->prev;
        m_list->prev = nullptr;
    }
    if (m_list->next != nullptr) {
        delete m_list->next;
        m_list->next = nullptr;
    }
    if (m_list->addr != nullptr) {
        m_list->addr = nullptr;
    }
    m_count = 0;
    return m_list;
}

template<typename Element>
inline int LinkedList<Element>::indexOf(Element elem)
{
    List* posp = m_list;
    for (int i = 0; i < m_count; i++) {
        if (posp->addr == elem)
            return i;
        posp = posp->next;
    }
    return -1;
}

template<typename Element>
inline bool LinkedList<Element>::isEmpty()
{
    return (m_list->prev == nullptr && m_list->addr == nullptr && m_list->next == nullptr);
}

template<typename Element>
inline void LinkedList<Element>::Delete(Element elem)
{
    this->remove(elem);
}

template<typename Element>
inline bool ListStack<Element>::is_empty()
{
    return (m_stack == nullptr || (m_stack->addr == nullptr && m_stack->next == nullptr));
}

template<typename Element>
inline void ListStack<Element>::dispose()
{
    if (m_stack != nullptr) {
        if (m_stack->next != nullptr)
            delete m_stack->next;
        delete m_stack;
        m_stack = nullptr;
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
    posp->next = m_stack;
    m_stack = posp;
    return m_stack;
}

template<typename Element>
inline Element ListStack<Element>::top()
{
    return Element(m_stack->addr);
}

template<typename Element>
inline void ListStack<Element>::pop()
{
    Stack* posp = m_stack->next;
    delete m_stack;
    m_stack = posp;
}

template<typename Element>
inline Tree* BinaryTree<Element>::MakeEmpty(PosPtr posp)
{
    if (posp != nullptr) {
        MakeEmpty(posp->left);
        MakeEmpty(posp->right);
        delete posp;
        posp = nullptr;
    }
    return posp;
}

template<typename Element>
inline Tree* BinaryTree<Element>::Find(Element elem, PosPtr posp)
{
    if (posp == nullptr) {
        return nullptr;
    }
    if (elem < posp->addr) {
        return this->Find(elem, posp->left);
    } else if (elem > posp->addr) {
        return this->Find(elem, posp->right);
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
        return this->Min(posp->left);
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
inline Tree* BinaryTree<Element>::Insert(Element elem, PosPtr& posp)
{
    if (posp == nullptr) {
        posp = new Tree();
        posp->addr = elem;
        posp->left = posp->right = nullptr;
    } else {
        if (elem < posp->addr) {
            posp->left = Insert(elem, posp->left);
        }
        if (elem > posp->addr) {
            posp->right = Insert(elem, posp->right);
        }
    }
    return posp;
}

template<typename Element>
inline Tree* BinaryTree<Element>::Delete(Element elem, PosPtr& posp)
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
            posp->right = Delete((Element)posp->addr, posp->right);
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
    return static_cast<Element>(posp->addr);
}

template<typename Element> void BinaryTree<Element>::PrintTree(PosPtr posp)
{
    if (posp != nullptr) {
        PrintTree(posp->left);
        printf("posp.addr: [%p]\n", posp->addr);
        PrintTree(posp->right);
    }
}

template<typename Element>
inline /*auto*/typename AVLBinTree<Element>::AvlTree
AVLBinTree<Element>::Insert(Element elem, typename AVLBinTree<Element>::AvlTree& posp)
// -> decltype(AVLBinTree<Element>::AvlTree)
{
    if (posp == nullptr) {
        posp = new Tree(elem);
        posp->height = 0;
        posp->left = posp->right = nullptr;
    } else {
        if (elem < posp->addr) {
            posp->left = Insert(elem, posp->left);
            if (Height(posp->left) - Height(posp->right) == 2) {
                if (elem < posp->left->addr)
                    posp = SingleRotateWithLeft(posp);
                else
                    posp = DoubleRotateWithLeft(posp);
            }
        }
        if (elem > posp->addr) {
            posp->right = Insert(elem, posp->right);
            if (Height(posp->right) - Height(posp->left) == 2) {
                if (elem < posp->right->addr)
                    posp = SingleRotateWithLeft(posp);
                else
                    posp = DoubleRotateWithLeft(posp);
            }
        }
    }
    if (posp != nullptr)
        posp->height = max(Height(posp->left), Height(posp->right)) + 1;
    return posp;
}

template<typename Element>
typename HashTable<Element>::Index HashTable<Element>::MakeHash(const char* key, int size)
{
    unsigned int value = 0;
    while (*key != '\0') {
        value = (value >> 5) + *key++;
    }
    return value % size;
}

template<typename Element>
typename HashTable<Element>::HashTbl HashTable<Element>::Initialize(int size)
{
    if (size < m_minSize) {
        return nullptr;
    }
    HashTbl h = new Hash();
    // h->size = NextPrime
    h->lists = (List*)malloc(sizeof(List) * h->size);
    if (h->lists != nullptr) {
        for (int i = 0; i < h->size; i++) {
            h->lists[i] = new List();
            h->lists[i].next = nullptr;
        }
    }
    return h;
}
