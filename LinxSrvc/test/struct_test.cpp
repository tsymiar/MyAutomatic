#include "test.h"
#include "Structures.h"

class TestCase {
public:
    int value;
};

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
}

TEST(Structures, BinaryTree)
{
    TestCase test0;
    BinaryTree<TestCase*> btree(&test0);
    BinaryTree<TestCase*>::PosPtr posp = new Tree();
    TestCase test1;
    test1.value = 1;
    Tree* tree = btree.Insert(&test1, posp);
    EXPECT_NE(nullptr, (long)tree);
    TestCase test2;
    test2.value = 2;
    EXPECT_EQ(btree.Insert(&test2, tree), tree);
    EXPECT_NE(btree.Find(&test1, tree), tree);
    EXPECT_EQ(btree.Max(tree)->addr, &test1);
    EXPECT_EQ((long)btree.Min(tree)->addr, nullptr);
    // EXPECT_EQ((long)btree.MakeEmpty(tree), nullptr);
    delete posp;
}
