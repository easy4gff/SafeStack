#define _SCL_SECURE_NO_WARNINGS

#include "Stack.h"
#include "unittest.h"

#include <iostream>

using namespace std;

int main() {
	// Тестирование ленивого копирования
    Stack<char> s1;
    s1.push('a');
    s1.push('b');
    s1.push('c');
    s1.push('d');
    s1.push('e');

    Stack<char> s2(s1);
    Stack<char> s3(s2);

    cout << "1)\n" << s1 <<  s2 << s3;
    char &rch = s1[0];
    Stack<char> s4(s1);
    cout << s4;
    rch = 'A';
    cout << "2)\n" << s1 << s2 << s3 << s4;
    s1.push('f');
    Stack<char> s5(s1);
    cout << "3)\n" << s1 << s2 << s3 << s4 << s5;
    s5.push('z');
    cout << "4)\n" << s1 << s2 << s3 << s4 << s5;
    s5.push('.');
    cout << "5)\n" << s1 << s2 << s3 << s4 << s5;
	cout << endl << endl << endl;

	// Тестирование стэка на соответствие строгой гарантии исключений
	testStrictExceptionGuarantee();


	// Общее тестирование стэка
	cout << "My tests." << endl;
	Stack<int> test_stack = createRandomStack<int>(100);
	assert(test_stack.size() == 100);

	Stack<int> test_stack1(test_stack);
	assert(test_stack1.size() == 100);
	try {
		for (size_t i = 0; i < 20; ++i) {
			test_stack1.top();
			test_stack1.pop();
		}
	}
	catch (const std::range_error & e) {
		cout << e.what() << endl;
	}
	catch (const std::out_of_range & e) {
		cout << e.what() << endl;
	}
	catch (...) {
		cout << "What the hell is going on?" << endl;
	}

	while (!test_stack.empty())
		test_stack.pop();
	assert(test_stack.size() == 0);


	Stack<int> st;

	// Push test
	for (int i = 0; i < 5; ++i)
		st.push(i);

	cout << "st: ";
	// Const iterator test
	for (Stack<int>::const_iterator it = st.cbegin(); it != st.cend(); ++it) {
		cout << *it << " ";
	}
	cout << endl;

	// Copy constructor test
	Stack<int> st1(st);
	cout << "st: " << st << endl;

	int num1 = 134;
	st1.push(num1);
	cout << "st: " << st << endl;

	// Iterator test
	cout << "st1: ";
	for (Stack<int>::const_iterator it = st1.cbegin(); it != st1.cend(); ++it) {
		cout << *it << " ";
	}
	cout << endl;

	// Operator= test
	Stack<int> st2 = st;
	//Stack<int> st2(st1);
	cout << "st2: " << st2 << endl;
	st2.pop();
	st2.pop();
	cout << "st2: " << st2;

	// Pop, top and empty methods test
	cout << "st (reversed): ";
	while (!st.empty()) {
		cout << st.top() << " ";
		st.pop();
	}
	cout << endl;
	cout << "st1: " << st1;

	/// Test push(const T&)
	NonMovebleInt num(42);
	Stack<NonMovebleInt> st_nm;
	st_nm.push(num);
	st_nm.push(num);
	st_nm.push(num);
	st_nm.push(num);
	st_nm.push(num);
	st_nm.push(num);
	cout << "Stack of NonMovableInt st_nm: " << st_nm << endl;

	Stack<NonMovebleInt> st_nm_2(st_nm);
	st_nm_2.push(NonMovebleInt(666));
	st_nm_2.push(NonMovebleInt(666));

	cout << endl;
	cout << "Stack of NonMovableInt st_nm: " << st_nm << endl;
	cout << "Stack of NonMovableInt st_nm_2: " << st_nm_2 << endl;
}
