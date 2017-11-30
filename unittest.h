#pragma once

#include "Stack.h"

#include <cassert>
#include <cstdlib>
#include <ctime>
#include <list>
#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

/// Неперемещаемый int
class NonMovebleInt {
public:
	/// Целочисленное значение
	int number;

	/// Конструктор
	NonMovebleInt(const int num) : number(num) {}

	/// Конструктор копирования
	NonMovebleInt(const NonMovebleInt & other) : number(other.number) {}

	/// Конструктор перемещения
	NonMovebleInt(NonMovebleInt &&) = delete;

	/// Перегруженная операция вывода в поток
	friend std::ostream & operator<<(std::ostream & os, const NonMovebleInt & nm) {
		os << nm.number;
		return os;
	}
};

struct Test
{
	int m_n;
	int m_nCheck;
	bool m_bThrowInMove, m_bThrowLater;
	static bool ms_bThrowDeferred;
	std::list <double> m_List;
	//
	Test(int n, bool bThrowInMove = false, bool bThrowLater = false)
		: m_n(n),
		m_nCheck(1),
		m_bThrowInMove(bThrowInMove),
		m_bThrowLater(bThrowLater),
		m_List(100, 1.)
	{
		cout << "Test(" << setw(3) << n << ")" << endl;
	}
	//
	Test(Test &&rrTest)
		: m_n(rrTest.m_n),
		m_nCheck(rrTest.m_nCheck),
		m_bThrowInMove(rrTest.m_bThrowInMove),
		m_bThrowLater(rrTest.m_bThrowLater),
		m_List(rrTest.m_List)
	{
		cout << "Test(Test &&rrTest), rrTest == " << rrTest << endl;
		if (m_bThrowInMove)
		{
			cout << "Throwing..." << endl;
			throw std::logic_error(string("Test(") + /*std::to_string(m_n)*/ + ")");
		}
		if (m_bThrowLater && ms_bThrowDeferred)
		{
			cout << "Throwing..." << endl;
			throw std::logic_error("Test(Test &&)");
		}
	}
	//
	friend std::ostream &operator << (std::ostream &rStream, const Test &rcTest)
	{
		if (rcTest.m_nCheck == 1)
			rStream << setw(3) << rcTest.m_n;
		else
			rStream << "invalid";
		//
		return rStream;
	}
	//
	Test(const Test &) = delete;
	Test &operator = (const Test &) = delete;
	Test &operator = (Test &&) = delete;
};

bool Test::ms_bThrowDeferred = false;

template<typename FwdIt>
void output(FwdIt begin, FwdIt end) {
	while (begin != end)
		cout << *begin++ << " ";
	cout << endl;
}

template<typename T>
void output(Stack<T> & st) {
	cout << st << "Capacity: " << st.capacity() << endl;
}

void output(const exception & e) {
	cout << e.what() << endl;
}

void testStrictExceptionGuarantee() {
	using Stack = Stack <Test>;
	//
	// 1. push()
	//
	Stack s1;
	int i = 0;
	for (; i < 10; ++i)
		s1.push(i);
	//
	for (; s1.size() < s1.capacity(); ++i)
		s1.push(i);
	//
	auto begin = s1.begin();
	auto end = s1.end();
	//
	output(s1);
	//
	try
	{
		s1.push(Test(999, true));
	}
	catch (const exception &rcException)
	{
		output(rcException);
	}
	//
	output(begin, end);
	//
	s1.push(i++);
	//
	begin = s1.begin();
	end = s1.end();
	//
	output(s1);
	//
	try
	{
		s1.push(Test(9999, true));
	}
	catch (const exception &rcException)
	{
		output(rcException);
	}
	//
	output(s1);
	output(begin, end);
	//
	s1.push(Test(i++, false, true));
	s1.push(i++);
	//
	output(s1);
	//
	// 2. Copy construction
	//
	Test::ms_bThrowDeferred = true;
	try
	{
		Stack s2 = s1;
	}
	catch (const exception &rcException)
	{
		output(rcException);
	}
	//
	// 3. Copy assignment
	//
	Stack s3;
	s3.push(111);
	begin = s3.begin();
	end = s3.end();
	try
	{
		s3 = s1;
	}
	catch (const exception &rcException)
	{
		output(rcException);
	}
	//
	output(s3);
	output(begin, end);
	cout << endl << endl;
}

template<typename T>
Stack<T> createRandomStack(size_t cnt_elems) {
	Stack<T> st;
	for (size_t i = 0; i < cnt_elems; ++i)
		st.push(rand() % 9999);
	assert(st.size() == cnt_elems);
	return st;
}



