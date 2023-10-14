#include "test.h"
#include "Structures.h"

class TestCase {
public:
    int value;
};

TEST(Structures, List)
{
    List list;
    List list1(0);
}

TEST(Structures, LinkedList)
{
    TestCase test0;
    LinkedList<TestCase*> link(&test0);
    EXPECT_EQ(1, link.size());
    TestCase test1;
    test1.value = 1;
    link.insert(1, &test1);
    ASSERT_EQ(2, link.size());
    TestCase test2;
    test2.value = 2;
    link.insert(2, &test2);
    ASSERT_EQ(3, link.size());

    List* list = link.get();
    EXPECT_EQ(list, link.first());
    EXPECT_NE(nullptr, (long)link.last());

    EXPECT_EQ(1, link.indexOf(&test1));
    EXPECT_EQ(true, link.contains(&test1));
    link.remove(&test1);
    EXPECT_EQ(false, link.contains(&test1));
    EXPECT_EQ(-1, link.indexOf(&test1));
    link.add(&test1);
    EXPECT_EQ(3, link.size());

    list = link.find_previous(&test2);
    ASSERT_NE(nullptr, (long)list);
    EXPECT_EQ(list->addr, &test0);

    TestCase test3;
    test3.value = 2;
    list = link.insert(&test3, list);
    EXPECT_EQ(list->next->addr, &test3);
    EXPECT_NE((long)link.add(&test1), nullptr);

    int step = 0;
    list = link.first();
    list = link.advance(list, ++step);
    if (list != nullptr) {
        EXPECT_EQ(link.retrieve(list), &test0);
        EXPECT_EQ(link.isEmpty(), false);
    }
    list = link.advance(list, ++step);
    if (list != nullptr) {
        EXPECT_EQ(link.retrieve(list), &test2);
        EXPECT_EQ(link.isEmpty(), true);
    }

    link.insert(0, nullptr);
    link.get(-1);
    link.advance(nullptr, -1);
    link.Delete(&test1);
}

TEST(Structures, ListStack)
{
    int value = 0;
    TestCase test0;
    ListStack<TestCase*> stacks(&test0);
    EXPECT_EQ(stacks.is_empty(), false);
    value++;
    TestCase test1;
    test1.value = 1;
    stacks.push(&test1);
    stacks.dispose();
    stacks.push(&test0);
    stacks.push(&test1);
    TestCase* test = stacks.top();
    EXPECT_EQ(test->value, value);
    value++;
    TestCase test2;
    test2.value = 2;
    stacks.push(&test2);
    test = stacks.top();
    EXPECT_EQ(test->value, 2);
    value++;
    TestCase test3;
    test3.value = 3;
    Stack* stack = stacks.push(&test3);
    test = stacks.top();
    EXPECT_EQ(test->value, 3);
    while (stack != nullptr && stack->next != nullptr) {
        EXPECT_EQ((reinterpret_cast<TestCase*>(stack->addr))->value, value--);
        stack = stack->next;
    }
    stacks.make_empty();
}

TEST(Structures, BinaryTree)
{
    TestCase test0;
    BinaryTree<TestCase*> bin_tree(&test0);
    BinaryTree<TestCase*>::PosPtr posp = new Tree();
    TestCase test1;
    test1.value = 1;
    Tree* tree = bin_tree.Insert(&test1, posp);
    EXPECT_NE(nullptr, (long)tree);
    TestCase test2;
    test2.value = 2;
    EXPECT_EQ(bin_tree.Insert(&test2, tree), tree);
    EXPECT_NE(bin_tree.Find(&test1, tree), tree);
    EXPECT_EQ(bin_tree.Retrieve(bin_tree.Max(tree)), &test1);
    EXPECT_EQ((long)bin_tree.Retrieve(bin_tree.Min(tree)), nullptr);
    TestCase test3;
    test3.value = 2;
    EXPECT_EQ(bin_tree.Insert(&test3, tree), tree);
    bin_tree.PrintTree(tree);
    EXPECT_EQ(bin_tree.Delete(&test1, tree), tree);
    EXPECT_EQ((long)bin_tree.MakeEmpty(tree), nullptr);
}

TEST(Structures, AVLBinTree)
{
    TestCase test0;
    AVLBinTree<TestCase*> avl_tree(&test0);
    TestCase test1;
    test1.value = 1;
    typename AVLBinTree<TestCase*>::AvlTree tree = new Tree();
    tree = avl_tree.Insert(&test1, tree);
    TestCase test2;
    test2.value = 2;
    tree = avl_tree.Insert(&test2, tree);
    avl_tree.PrintTree(tree);
    EXPECT_EQ((long)avl_tree.MakeEmpty(tree), nullptr);
    AVLBinTree<TestCase*>::DoubleRotateWithLeft(tree);
}
