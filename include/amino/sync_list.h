/*
 * (c) Copyright 2008, IBM Corporation.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef SYNC_LIST_H_
#define SYNC_LIST_H_

#include "lock.h"
#include "mutex.h"
#include <list>
#include <algorithm>

namespace amino {
/**
 * @brief This is a implementation of lock based list. It is simply added lock before every
 * operation of ordinary list(such as the std::list<T>).
 *
 * @author Dai Xiao Jun
 *
 * @tparam T
 * 			The type of the element which is stored in the list
 * @tparam Sequence
 * 			The internal list's type which take the std::list<T> as the default value. *
 */
template<class T, class Sequence = std::list<T> >
class SyncList {
private:
	/**
	 * internal list, usually is not thread-safe.
	 */
	Sequence internalList;
	/**
	 * The mutex is used to keep the operations on the list be sequenced.
	 */
	mutex lock;

public:
	typedef	typename Sequence::iterator iterator;
	typedef	typename Sequence::const_iterator const_iterator;
	typedef typename Sequence::size_type size_type;
	typedef typename Sequence::reference reference;
	typedef typename Sequence::const_reference const_reference;
	typedef typename Sequence::value_type value_type;

	/**
	 * @brief Judge if the list is empty
	 *
	 * @return
	 * 			If the list is empty, return true, else return false.
	 */
	bool empty() {
		unique_lock<mutex> common_lock(lock);
		return internalList.empty();
	}

	/**
	 * @brief Get the size of list
	 *
	 * @return
	 * 			The size of list
	 */
	size_type size() {
		unique_lock<mutex> common_lock(lock);
		return internalList.size();
	}

	/**
	 * @brief Get the first element in the list. If the list is empty, it will return false, else return
	 * true and assign the first element to the parameter.
	 *
	 * @param ret
	 * 			The first element in the list. It is valid if the return value is true.
	 * @return
	 * 			If the list is not empty, return true, else return false
	 */
	bool front(reference ret) {
		unique_lock<mutex> common_lock(lock);
		ret = internalList.front();
		return true;
	}

	/**
	 * @brief Get element without range check
	 *
	 * @param index
	 * 			The position of the element
	 * @return
	 * 			the element in the right position
	 */
	T& operator[](int index) {
		unique_lock<mutex> common_lock(lock);
	}

	/**
	 * @brief Get element with range check
	 *
	 * @param index
	 * 			The position of the element
	 * @return
	 * 			The element in the right position
	 */
	T& at(int index) {
		unique_lock<mutex> common_lock(lock);
	}

	/**
	 * @brief Add the specified element into the list
	 *
	 * @param index
	 * 			The position to be added to
	 * @param value
	 * 			The element to be added
	 *
	 * @return
	 * 			If the element inserted successful return true, else return false
	 */
	bool insert(int index, const T& e) {
		unique_lock<mutex> common_lock(lock);
		iterator ite = internalList.begin();
		for(int i = 0; i < index; ++i) {
			if(ite == internalList.end()) {
				break;
			}
			ite++;
		}

		if(ite != internalList.end()) {
			internalList.insert(ite, e);
			return true;
		} else {
			return false;
		}
	}

	/**
	 * @brief Add a specified element to be the first element of the list
	 *
	 * @return
	 * 			true for success and false for failed
	 */
	bool push_front(const T& x) {
		unique_lock<mutex> common_lock(lock);
		internalList.push_front(x);
		return true;
	}

	/**
	 * @brief Add a specified element to be the last element of the list
	 *
	 * @return
	 * 			True for success and false for failed
	 */
	bool push_back(const T& x) {
		unique_lock<mutex> common_lock(lock);
		internalList.push_back(x);
		return true;
	}

	/**
	 * @brief Pop the first element of the list
	 */
	void pop_front() {
		unique_lock<mutex> common_lock(lock);
		internalList.pop_front();
	}

	/**
	 * @brief Pop the last element of the list
	 */
	void pop_back() {
		unique_lock<mutex> common_lock(lock);
		internalList.pop_back();
	}

	/**
	 * @brief Erases all of the elements
	 */
	void clear() {
		unique_lock<mutex> common_lock(lock);
		internalList.clear();
	}

	/**
	 * @brief Remove one element. different from remove in stl, which remove all the same elements
	 *
	 * @param value
	 * 			The element to be removed
	 * @return
	 * 			If the value exists in the list, remove it and return true. else return false.
	 */
	bool remove(const T& value) {
		unique_lock<mutex> common_lock(lock);
		iterator ite = find(internalList.begin(), internalList.end(), value);
		if(ite != internalList.end()) {
			internalList.erase(ite);
			return true;
		} else {
			return false;
		}
	}

	/**
	 * @brief Removes all but the first element in every consecutive group of equal elements.
	 */
	void unique() {
		unique_lock<mutex> common_lock(lock);
		internalList.unique();
	}

	/**
	 * @brief Both *this and x must be sorted according to operator<, and they must be distinct.
	 * (That is, it is required that &x != this.) This function removes all of x's elements and
	 * inserts them in order into *this.
	 *
	 * @brief x
	 * 			Another list
	 */
	void merge(Sequence& x) {
		unique_lock<mutex> common_lock(lock);
		internalList.merge(x);
	}

	/**
	 * @brief Reverses the order of elements in the list. All iterators remain valid and continue to
	 * point to the same elements
	 */
	void reverse() {
		unique_lock<mutex> common_lock(lock);
		internalList.reverse();
	}

	/**
	 * @brief Sort the list
	 */
	void sort() {
		unique_lock<mutex> common_lock(lock);
		internalList.reverse();
	}

	iterator begin() {
		return internalList.begin();
	}

	iterator end() {
		return internalList.begin();
	}

	const_iterator begin() const {
		return internalList.begin();
	}

	const_iterator end() const {
		return internalList.begin();
	}

};

};
#endif /* SYNC_LIST_H_ */
