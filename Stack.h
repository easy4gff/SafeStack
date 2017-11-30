#pragma once

#include <algorithm>
#include <iostream>
#include <climits>
#include <utility>
#include <stdexcept>
#include <type_traits>

template<typename It>
void printContainer(It begin, It end) {
	while (begin != end) {
		std::cout << *begin << " ";
		++begin;
	}
	std::cout << std::endl;
}

template<typename T1, typename T2>
typename std::enable_if<std::is_move_constructible<T1>::value>::type construct(T1 * dest, T2 && elem) {
	new (dest)(T1)(std::move(elem));
}

template<typename T1, typename T2>
typename std::enable_if<!std::is_move_constructible<T1>::value>::type construct(T1 * dest, const T2 & elem) {
	std::cout << "Constructing non-movable elem..." << std::endl;
	new (dest)(T1)(elem);
}

template<typename T>
void destroy(T * elem) {
	elem->~T();
}

template<typename FwdIter>
void destroy(FwdIter it1, FwdIter it2) {
	while (it1 != it2) {
		destroy(&*it1);
		++it1;
	}
}

template<typename T>
class ArrImpl {
public:
	/// ��������� �������
	const static int INITIAL_MAX_SIZE = 5;
	/// ��������� ���������� ������� ��� ���������� reserve
	const static int RESIZE_FACTOR = 2;
	/// ��������� ��������������
	const static size_t UNSHAREABLE = INT_MAX;

	/// ������ ������
	T * _data;
	/// ������� ������
	size_t _cur_size;
	/// �������
	size_t _max_size;
	/// ���������� ������
	size_t _count_refs;

	/// ����������� �� ���������
	explicit ArrImpl(size_t sz = ArrImpl<T>::INITIAL_MAX_SIZE) :
		_data(sz == 0 ? 0 : static_cast<T*>(operator new (sz * sizeof(T)))),
		_cur_size(0),
		_max_size(sz),
		_count_refs(1)
	{}

	/// ������� ������
	void swap(ArrImpl & other) {
		std::swap(_data, other._data);
		std::swap(_cur_size, other._cur_size);
		std::swap(_max_size, other._max_size);
		std::swap(_count_refs, other._count_refs);
	}

	/// ���������� � �� ArrImpl, ���� � ���� � ���� ����������� ����������� (����� ��� ������ ���������������
	/// ����� ������ �-�� construct(T1*, T2&&).
	template<typename T1>
	static typename std::enable_if<std::is_move_constructible<T1>::value, T1&&>::type MoveOrPassRef(T1 && val) {
		return val;
	}

	/// ���������� � �� ArrImpl, ���� � ���� � ��� ������������ ����������� (����� ��� ������ ���������������
	/// ����� ������ �-�� construct(T1*, T2&).
	template<typename T1>
	static typename std::enable_if<!std::is_move_constructible<T1>::value, const T1&>::type MoveOrPassRef(const T1 & val) {
		return val;
	}


	/// ����������� �����
	ArrImpl(const ArrImpl<T> & arr) {
		_data = arr._max_size == 0 ? 0 : static_cast<T*>(operator new (arr._max_size * sizeof(T)));
		_max_size = arr._max_size;
		_cur_size = 0;
		_count_refs = 1;
		while (this->_cur_size != arr._cur_size) {
			construct(_data + _cur_size, MoveOrPassRef(*(arr._data + _cur_size)));
			++_cur_size;
		}
	}

	/// �������� ������������ (�� ����� �����)
	/*ArrImpl & operator=(ArrImpl arr) {
		swap(arr);
		return *this;
	}*/

	/// ����������
	~ArrImpl() {
		destroy(_data, _data + _cur_size);
		operator delete (_data);
	}
};

/// ������ ������������� �������
template<typename T>
class DynArr {
public:
	/// Cc���� �� ������ ������ ArrImpl
	ArrImpl<T> * _impl;
public:
	/// �������� ������������� �������
	class iterator
	{
	private:
		T * _data;
	public:
		iterator(T * data) : _data(data) {}

		bool operator==(const iterator & it) const {
			return _data == it._data;
		}

		bool operator!=(const iterator & it) const {
			return !(*this == it);
		}

		T & operator*() {
			return *_data;
		}

		iterator operator++() {
			++_data;
			return *this;
		}

		iterator operator++(int) {
			iterator result(_data);
			++this;
			return result;
		}
	};

	/// ����������� �� ���������
	DynArr(size_t initial_sz = ArrImpl<T>::INITIAL_MAX_SIZE) : _impl(new ArrImpl<T>(initial_sz)) {}

	/// ����������� �������������
	DynArr(const DynArr & arr) {
		if (arr._impl->_count_refs != ArrImpl<T>::UNSHAREABLE) {
			_impl = arr._impl;
			_impl->_count_refs++;
		}
		else
			_impl = new ArrImpl<T>(*arr._impl);
	}

	/// ����������� �����������
	DynArr(DynArr && arr) {
		_impl = arr._impl;
		arr._impl = nullptr;
	}

	/// �������� ������������
	DynArr & operator=(const DynArr & arr) {
		if (arr._impl->_count_refs != ArrImpl<T>::UNSHAREABLE) {
			_impl = arr._impl;
			_impl->_count_refs++;
		}
		else {
			ArrImpl<T> * new_impl = new ArrImpl<T>(*arr._impl);

			if (_impl->_count_refs > 1) {
				_impl->_count_refs--;
			}
			else {
				delete _impl;
			}
			_impl = new_impl;
		}
		return *this;
	}

	/// ������� ������
	void swap(DynArr<T> & other) {
		std::swap(_impl, other._impl);
	}

	/// �������� ������������ (������)
	DynArr & operator=(DynArr && arr) {
		swap(arr);
		return *this;
	}

	/// ���������� �������� (�������������) � �����
	void push_back(T && elem) {
		deep_copy();
		_push_back(std::move(elem));
	}

	/// ���������� �������� (�� �������������) � �����
	void push_back(const T & elem) {
		deep_copy();
		_push_back(elem);
	}

	/// ������� �������� �� �����
	void delete_back() {
		deep_copy();
		destroy(_impl->_data + size());
		--_impl->_cur_size;
	}

	/// ������
	size_t size() const {
		return _impl->_cur_size;
	}

	/// ������������ ������
	size_t capacity() const {
		return _impl->_max_size;
	}

	/// �������� �� �������
	bool empty() const {
		return _impl->_cur_size == 0;
	}

	/// ������ �� ������� (������ �� ������)
	T operator[](size_t index) const {
		return _impl->_data[index];
	}

	/// ������ �� �������
	T & operator[](size_t index) {
        deep_copy(true);
		return _impl->_data[index];
	}

	/// �������� ������
	iterator begin() {
		return iterator(_impl->_data);
	}

	/// �������� �����
	iterator end() {
		return iterator(_impl->_data + _impl->_cur_size);
	}

	/// ������������� �������� ������ � �����
	friend std::ostream & operator<<(std::ostream & os, DynArr<T> & st) {
		for (T x : st) {
			os << x << " ";
		}
		os << std::endl;
		return os;
	}

	/// ����������
	~DynArr() {
		if (_impl) {
			if (_impl->_count_refs == 1 || _impl->_count_refs == ArrImpl<T>::UNSHAREABLE)
				delete _impl;
			else
				--_impl->_count_refs;
		}
	}

private:
	/// �������� ����������� ������
	void deep_copy(bool markUnshareable = false, bool allow_shorten = false) {
		if (_impl->_count_refs > 1 && _impl->_count_refs != ArrImpl<T>::UNSHAREABLE) {
			ArrImpl<T> * old_impl = _impl;
			_impl = new ArrImpl<T>(*_impl);
			--old_impl->_count_refs;
		}

        _impl->_count_refs = markUnshareable ? ArrImpl<T>::UNSHAREABLE : 1;
	}

	/// ��������������� ������� ��� ���������� �������� (�������������)
	void _push_back(T && elem) {
		if (size() == _impl->_max_size) {
			DynArr arr(size() * ArrImpl<T>::RESIZE_FACTOR);
			while (arr.size() != size()) {
				arr._push_back(std::move(*(_impl->_data + arr.size())));
			}
			arr.push_back(std::move(elem));
			swap(arr);
		}
		else {
			construct(_impl->_data + size(), std::move(elem));
			++_impl->_cur_size;
		}
	}

	/// ��������������� ������� ��� ���������� �������� (�� �������������)
	void _push_back(const T & elem) {
		if (size() == _impl->_max_size) {
			DynArr arr(size() * ArrImpl<T>::RESIZE_FACTOR);
			while (arr.size() != size()) {
				arr._push_back(*(_impl->_data + arr.size()));
			}
			arr.push_back(elem);
			swap(arr);
		}
		else {
			construct(_impl->_data + size(), elem);
			++_impl->_cur_size;
		}
	}
};

/// ������ ������ �����
template<typename T>
class Stack {
private:
	/// ������������ ������
	DynArr<T> _data;

public:
	/// ����� ���������
	class iterator
	{
	private:
		T * _data;
	public:
		iterator(T * data) : _data(data) {}

		bool operator==(const iterator & it) const {
			return _data == it._data;
		}

		bool operator!=(const iterator & it) const {
			return !(*this == it);
		}

		T & operator*() {
			return *_data;
		}

		T operator*() const {
			return *_data;
		}

		iterator operator++() {
			++_data;
			return *this;
		}

		iterator operator++(int) {
			iterator result(_data);
			++_data;
			return result;
		}
	};

	/// ����� ������������ ���������
	class const_iterator : public iterator {
	public:
		const_iterator(T * elem) : iterator(elem) {}

		T operator*() {
			return iterator::operator*();
		}
	};

	/// ����������� �� ���������
	Stack() {}

	/// ����������� �����������
	Stack(const Stack<T> & st) : _data(st._data) {}

	/// ����������� �����������
	Stack(Stack && st) : _data(std::move(st._data)) {}

	/// �������� �����������
    Stack & operator=(const Stack & other) {
		_data = other._data;
		return *this;
	}

	/// ���������� �������� T (�� �������������)
	void push(const T & elem) {
		//T tmp(elem);
		_data.push_back(elem);
	}

	/// ���������� �������� T (�������������)
	void push(T && elem) {
		_data.push_back(std::move(elem));
	}

	/// ���������� ������� ������� �����
	T top() const {
		if (empty())
			throw std::out_of_range("Stack is empty!");
		return _data[_data.size() - 1];
	}

	/// ������� ������� � �������� �����
	void pop() {
		if (empty())
			throw std::out_of_range("Stack is empty!");
		_data.delete_back();
	}

	/// ���������� ������
	size_t size() const {
		return _data.size();
	}

	/// ������������ ������
	size_t capacity() const {
		return _data.capacity();
	}

	/// ��������� �� ���������
	bool empty() const {
		return _data.empty();
	}

	/// ���������� �������� ������
	iterator begin() {
		iterator it(&*_data.begin());
		return it;
	}

	/// ���������� �������� �����
	iterator end() {
		iterator it(&*_data.end());
		return it;
	}

	/// ���������� �������� ������
	const_iterator cbegin() {
		const_iterator it(&*_data.begin());
		return it;
	}

	/// ���������� �������� �����
	const_iterator cend() {
		const_iterator it(&*_data.end());
		return it;
	}

	/// �������� ������� �� �������
    T operator[](size_t index) const {
        return _data[index];
    }

	/// �������� ������� �� ������� �� ������
    T & operator[](size_t index) {
        return _data[index];
    }

	/// �������� ������ � �����
    friend std::ostream & operator<<(std::ostream & os, Stack<T> & st) {
        for (T & x : st) {
            os << x << " ";
        }
		os << std::endl;
        return os;
    }
};
