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
	/// Начальная ёмкость
	const static int INITIAL_MAX_SIZE = 5;
	/// Множитель увеличения размера при выполнении reserve
	const static int RESIZE_FACTOR = 2;
	/// Индикатор неразделимости
	const static size_t UNSHAREABLE = INT_MAX;

	/// Массив данных
	T * _data;
	/// Текущий размер
	size_t _cur_size;
	/// Ёмкость
	size_t _max_size;
	/// Количество ссылок
	size_t _count_refs;

	/// Конструктор по умолчанию
	explicit ArrImpl(size_t sz = ArrImpl<T>::INITIAL_MAX_SIZE) :
		_data(sz == 0 ? 0 : static_cast<T*>(operator new (sz * sizeof(T)))),
		_cur_size(0),
		_max_size(sz),
		_count_refs(1)
	{}

	/// Функция обмена
	void swap(ArrImpl & other) {
		std::swap(_data, other._data);
		std::swap(_cur_size, other._cur_size);
		std::swap(_max_size, other._max_size);
		std::swap(_count_refs, other._count_refs);
	}

	/// Вызывается в КК ArrImpl, если у типа Т есть конструктор перемещения (нужно для вызова соответствующей
	/// этому случаю ф-ии construct(T1*, T2&&).
	template<typename T1>
	static typename std::enable_if<std::is_move_constructible<T1>::value, T1&&>::type MoveOrPassRef(T1 && val) {
		return val;
	}

	/// Вызывается в КК ArrImpl, если у типа Т нет конструктора перемещения (нужно для вызова соответствующей
	/// этому случаю ф-ии construct(T1*, T2&).
	template<typename T1>
	static typename std::enable_if<!std::is_move_constructible<T1>::value, const T1&>::type MoveOrPassRef(const T1 & val) {
		return val;
	}


	/// Конструктор копии
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

	/// Оператор присваивания (не нужен здесь)
	/*ArrImpl & operator=(ArrImpl arr) {
		swap(arr);
		return *this;
	}*/

	/// Деструктор
	~ArrImpl() {
		destroy(_data, _data + _cur_size);
		operator delete (_data);
	}
};

/// Шаблон динамического массива
template<typename T>
class DynArr {
public:
	/// Ccылка на объект класса ArrImpl
	ArrImpl<T> * _impl;
public:
	/// Итератор динамического массива
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

	/// Конструктор по умолчанию
	DynArr(size_t initial_sz = ArrImpl<T>::INITIAL_MAX_SIZE) : _impl(new ArrImpl<T>(initial_sz)) {}

	/// Конструктор инициализации
	DynArr(const DynArr & arr) {
		if (arr._impl->_count_refs != ArrImpl<T>::UNSHAREABLE) {
			_impl = arr._impl;
			_impl->_count_refs++;
		}
		else
			_impl = new ArrImpl<T>(*arr._impl);
	}

	/// Конструктор перемещения
	DynArr(DynArr && arr) {
		_impl = arr._impl;
		arr._impl = nullptr;
	}

	/// Оператор присваивания
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

	/// Функция обмена
	void swap(DynArr<T> & other) {
		std::swap(_impl, other._impl);
	}

	/// Оператор присваивания (обмена)
	DynArr & operator=(DynArr && arr) {
		swap(arr);
		return *this;
	}

	/// Добавление элемента (перемещаемого) в конец
	void push_back(T && elem) {
		deep_copy();
		_push_back(std::move(elem));
	}

	/// Добавление элемента (не перемещаемого) в конец
	void push_back(const T & elem) {
		deep_copy();
		_push_back(elem);
	}

	/// Удалния элемента из конца
	void delete_back() {
		deep_copy();
		destroy(_impl->_data + size());
		--_impl->_cur_size;
	}

	/// Размер
	size_t size() const {
		return _impl->_cur_size;
	}

	/// Максимальный размер
	size_t capacity() const {
		return _impl->_max_size;
	}

	/// Проверка на пустоту
	bool empty() const {
		return _impl->_cur_size == 0;
	}

	/// Доступ по индексу (только на чтение)
	T operator[](size_t index) const {
		return _impl->_data[index];
	}

	/// Доступ по индексу
	T & operator[](size_t index) {
        deep_copy(true);
		return _impl->_data[index];
	}

	/// Итератор начала
	iterator begin() {
		return iterator(_impl->_data);
	}

	/// Итератор конца
	iterator end() {
		return iterator(_impl->_data + _impl->_cur_size);
	}

	/// Перегруженная операция вывода в поток
	friend std::ostream & operator<<(std::ostream & os, DynArr<T> & st) {
		for (T x : st) {
			os << x << " ";
		}
		os << std::endl;
		return os;
	}

	/// Деструктор
	~DynArr() {
		if (_impl) {
			if (_impl->_count_refs == 1 || _impl->_count_refs == ArrImpl<T>::UNSHAREABLE)
				delete _impl;
			else
				--_impl->_count_refs;
		}
	}

private:
	/// Глубокое копирование данных
	void deep_copy(bool markUnshareable = false, bool allow_shorten = false) {
		if (_impl->_count_refs > 1 && _impl->_count_refs != ArrImpl<T>::UNSHAREABLE) {
			ArrImpl<T> * old_impl = _impl;
			_impl = new ArrImpl<T>(*_impl);
			--old_impl->_count_refs;
		}

        _impl->_count_refs = markUnshareable ? ArrImpl<T>::UNSHAREABLE : 1;
	}

	/// Вспомогательная функция для добавления элемента (перемещаемого)
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

	/// Вспомогательная функция для добавления элемента (не перемещаемого)
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

/// Шаблон класса стэка
template<typename T>
class Stack {
private:
	/// Динамический массив
	DynArr<T> _data;

public:
	/// Класс итератора
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

	/// Класс константного итератора
	class const_iterator : public iterator {
	public:
		const_iterator(T * elem) : iterator(elem) {}

		T operator*() {
			return iterator::operator*();
		}
	};

	/// Конструктор по умолчанию
	Stack() {}

	/// Конструктор копирования
	Stack(const Stack<T> & st) : _data(st._data) {}

	/// Конструктор перемещения
	Stack(Stack && st) : _data(std::move(st._data)) {}

	/// Оператор перемещения
    Stack & operator=(const Stack & other) {
		_data = other._data;
		return *this;
	}

	/// Добавление элемента T (не перемещаемого)
	void push(const T & elem) {
		//T tmp(elem);
		_data.push_back(elem);
	}

	/// Добавление элемента T (перемещаемого)
	void push(T && elem) {
		_data.push_back(std::move(elem));
	}

	/// Возвращает верхний элемент стэка
	T top() const {
		if (empty())
			throw std::out_of_range("Stack is empty!");
		return _data[_data.size() - 1];
	}

	/// Убирает элемент с верхушки стэка
	void pop() {
		if (empty())
			throw std::out_of_range("Stack is empty!");
		_data.delete_back();
	}

	/// Возвращает размер
	size_t size() const {
		return _data.size();
	}

	/// Максимальный размер
	size_t capacity() const {
		return _data.capacity();
	}

	/// Проверяет на непустоту
	bool empty() const {
		return _data.empty();
	}

	/// Возвращает итератор начала
	iterator begin() {
		iterator it(&*_data.begin());
		return it;
	}

	/// Возвращает итератор конца
	iterator end() {
		iterator it(&*_data.end());
		return it;
	}

	/// Возвращает итератор начала
	const_iterator cbegin() {
		const_iterator it(&*_data.begin());
		return it;
	}

	/// Возвращает итератор конца
	const_iterator cend() {
		const_iterator it(&*_data.end());
		return it;
	}

	/// Оператор доступа по индексу
    T operator[](size_t index) const {
        return _data[index];
    }

	/// Оператор доступа по индексу по ссылке
    T & operator[](size_t index) {
        return _data[index];
    }

	/// Оператор вывода в поток
    friend std::ostream & operator<<(std::ostream & os, Stack<T> & st) {
        for (T & x : st) {
            os << x << " ";
        }
		os << std::endl;
        return os;
    }
};
