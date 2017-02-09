#include <ios>

template <class Class>
class MyTest
{
public:
	MyTest<Class>();
	virtual ~MyTest<Class>();
	Class* Obj;
};

template <class Class>
MyTest<Class>::MyTest()
{
	Obj=new Class();
}

template <class Class>
MyTest<Class>::~MyTest()
{
	delete Obj;
}
