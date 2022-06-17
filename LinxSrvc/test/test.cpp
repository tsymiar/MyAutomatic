#include "gtest.h"
#include "Structures.h"

#include <iostream>
using namespace std;

class TestCase {
public:
    int value;
};

TEST(test_fixture, LinkedList)
{
    TestCase test0;
    LinkedList<TestCase *> link(&test0);
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
	EXPECT_NE(nullptr, (long)link.first());
	EXPECT_NE(nullptr, (long)link.last());
}
 
int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
