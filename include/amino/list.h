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

#ifndef AMINO_LIST_H
#define AMINO_LIST_H
#include <iostream>
#include <stdexcept>
#include <iterator>
#include "smr.h"

namespace amino {

/*internal namespace for SMR*/
using namespace internal;
/*
 * Number of pointers protected by SMR.
 */
#define NHPOINTER 3
/* clear the last two bits of pointers */
#define POINTER(p) ((NodeType<KeyType>*)(((long)(p))&(~3)))
/* set the last bit of pointers */
#define MARK(p) ((NodeType<KeyType>*)(((long)(p))|(1)))
/* get the last bit of pointers */
#define MARKED(p) (((long)(p))&(1))

/**
 * @brief The node type, which stores the data and a next pointer in it.
 *
 * @tparam KeyType
 * 				Type of element stored in it
 */
template<typename KeyType> class NodeType {
public:
	/*the data*/
	KeyType data;

	/*pointer to the next node*/
	atomic<NodeType<KeyType>*> next;

	/**
	 * @brief Constructor. construct a node
	 */
	NodeType() {
		next.store(NULL, memory_order_relaxed);
	}

	/**
	 * @brief Constructor. construct a node
	 * @param val
	 * 			A specified element
	 */
	NodeType(const KeyType& val) :
		data(val) {
		next.store(NULL, memory_order_relaxed);
	}
};

/**
 * @brief a state holder, which stores the pointer of the current node and the pointers of the current
 * node's previous and next in it.
 *
 * @tparam KeyType
 * 				Type of element stored in the NodeType
 */
template<typename KeyType> class FindStateHolder {
public:
	atomic<NodeType<KeyType>*>* prev;
	NodeType<KeyType>* cur;
	NodeType<KeyType>* next;
	bool isFound;

	FindStateHolder() :
		prev(NULL), cur(NULL), next(NULL), isFound(false) {
	}
};

/**
 * @brief The iterator of the list.
 * @tparam T
 * 				Type of element stored in the list
 */
template<typename T> struct __list_iterator {
	typedef __list_iterator<T> _Self;
	typedef NodeType<T> _Node;

	typedef ptrdiff_t difference_type;
	typedef std::forward_iterator_tag iterator_category;
	typedef T value_type;
	typedef T* pointer;
	typedef T& reference;

	NodeType<T>* node;

	__list_iterator(_Node* x) :
		node(x) {
	}

	__list_iterator(const _Self& x) :
		node(x.node) {
	}

	__list_iterator() {
	}

	_Self& operator ++() {
		node = node->next.load(memory_order_relaxed);
		return *this;
	}

	_Self operator ++(int) {
		_Self tmp = *this;
		++*this;
		return tmp;
	}

	bool operator ==(const _Self & x) const {
		return node == x.node;
	}
	bool operator !=(const _Self & x) const {
		return node != x.node;
	}

	reference operator *() const {
		return node->data;
	}

	pointer operator ->() const {
		return &(node->data);
	}

};

/**
 * @brief The const iterator of the list.
 * @tparam T
 * 				Type of element stored in the list
 */
template<typename T> struct __list_const_iterator {
	typedef __list_const_iterator<T> _Self;
	typedef const NodeType<T> _Node;
	typedef __list_iterator<T>                iterator;

	typedef ptrdiff_t difference_type;
	typedef std::forward_iterator_tag iterator_category;
	typedef T value_type;
	typedef const T* pointer;
	typedef const T& reference;

	const NodeType<T>* node;

	__list_const_iterator(const _Node* x) :
		node(x) {
	}

	__list_const_iterator(const _Self& x) :
		node(x.node) {
	}

	__list_const_iterator(const iterator& x)
    : node(x.node) { }

	__list_const_iterator() {
	}

	_Self& operator ++() {
		node = node->next.load(memory_order_relaxed);
		return *this;
	}

	_Self operator ++(int) {
		_Self tmp = *this;
		++*this;
		return tmp;
	}

	bool operator ==(const _Self & x) const {
		return node == x.node;
	}
	bool operator !=(const _Self & x) const {
		return node != x.node;
	}

	reference operator *() const {
		return node->data;
	}

	pointer operator ->() const {
		return &(node->data);
	}

};

template<typename T>
inline bool operator==(const __list_iterator<T>& lhs,
		const __list_const_iterator<T>& rhs) {
	return lhs.node == rhs.node;
}

template<typename T>
inline bool operator!=(const __list_iterator<T>& lhs,
		const __list_const_iterator<T>& rhs) {
	return lhs.node != rhs.node;
}

/**
 * @brief This is an implementation of a lock-free linked list data structure. The
 * implementation is according to the paper <a
 * href="http://www.research.ibm.com/people/m/michael/spaa-2002.pdf"> High
 * Performance Dynamic Lock-Free Hash Tables and List-Based Sets</a> by Maged M.
 * Michael, 2002. To gain a complete understanding of this data structure,
 * please first read this paper, available at:
 * http://www.research.ibm.com/people/m/michael/spaa-2002.pdf
 * <p>
 * This lock-free linked list is an unbounded thread-safe linked list. A
 * <tt>LockFreeList</tt> is an appropriate choice when many threads will share
 * access to a common collection. This list does not permit <tt>null</tt>
 * elements.
 * <p>
 * This is a lock-free implementation intended for highly scalable add, remove
 * and contains which is thread safe. Add() will add the element to the head of the list which is
 * different with the normal list.
 *
 * <p>
 * Iterator is not thread-safe and only used when debugging.
 *
 * <p>
 * ABA prevention method is based on Maged M. Michael's:
 * Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects
 *
 * @author Xiao Jun Dai
 *
 * @tparam KeyType
 * 			The type of the element which is stored in the list
 *
 */
template<typename KeyType>
class List {
public:
	typedef __list_iterator<KeyType> iterator;
	typedef __list_const_iterator<KeyType> const_iterator;

protected:
	/**
	 * Pointer to header node, initialized by a dummy node.
	 */
	atomic<NodeType<KeyType>*> head;
	/**
	 *a pointer created for initializing of head pointer, for the performance reason
	 */
	NodeType<KeyType>* dummy;

	/**
	 * for memory management
	 */
	SMR<NodeType<KeyType>, NHPOINTER>* mm;
    typedef typename SMR<NodeType<KeyType>, NHPOINTER>::HP_Rec HazardP;

	/* prevent the use of copy constructor and copy assignment*/
	List(const List& s) {
	}

	List& operator=(const List& st) {
	}

public:
	/**
	 *  @brief constructor. constructs a empty with head is dummy
	 */
	List() {
		mm = getSMR<NodeType<KeyType> , NHPOINTER> (); //for memory management, 1 hazard pointer is OK
		dummy = new NodeType<KeyType> ();
        assert(NULL != dummy);
		head.store(dummy->next.load(memory_order_relaxed), memory_order_relaxed);
	}

	/**
	 * @brief Destructor, supposed to be called when its associated data structure is to be deleted
	 */
	~List() {
		NodeType<KeyType>* tmp = head.load(memory_order_relaxed);
		while (tmp != NULL) {/* if the list is not empty. delete the node of it until it is empty*/
			NodeType<KeyType>*tmp_keep = POINTER(tmp);
			assert(NULL != tmp_keep);
            tmp = tmp_keep->next.load(memory_order_relaxed);
            assert(!MARKED(tmp_keep));
			delete tmp_keep;
		}

		delete dummy;
	}

	/**
	 * @brief Get the first element in the list. If the list is empty, it will return false.
	 * else return true and assign the first element to the parameter.
	 *
	 * @param ret
	 * 			The first element in the list. It is valid if the return value is true.
	 * @return
	 * 			If the list is empty return false, else return true.
	 */
	bool front(KeyType& ret) {
        HazardP * hp = mm->getHPRec();
            
        NodeType<KeyType>* first = head.load(memory_order_relaxed);
        while(true)
        {
            if (first == NULL)
                return false;
            
            if (MARKED(first))
            { 
                mm->employ(hp, 0, first);
                if (first != head.load(memory_order_relaxed))
                {
                    first = head.load(memory_order_relaxed);
                    continue;
                }

                first = first->next.load(memory_order_relaxed);
                mm->retire(hp, 0);
                continue;
            }
            assert(!MARKED(first));
            ret = first->data;
            return true;
        }
    }

	/**
	 * @brief Add the specified element into the list
	 *
	 * @param index
	 * 			Not work in this function now.
	 * @param e
	 * 			The element to be added
	 *
	 * @return
	 * 			If the element inserted successful return true, else return false
	 */
	bool insert(int index, const KeyType& e) {
		return add(e);
	}

	iterator begin() {
		return iterator(head.load(memory_order_relaxed));
	}

	iterator end() {
		return iterator(NULL);
	}

	const_iterator begin() const {
		return const_iterator(head.load(memory_order_relaxed));
	}

	const_iterator end() const {
		return const_iterator(NULL);
	}

	/**
	 * @brief Get the size of list
	 *
	 * @return
	 * 			The size of list
	 */
	int size() {
		int ret = 0;
		NodeType<KeyType>* curr = head.load(memory_order_relaxed);
		while (curr != NULL) {
			if (!MARKED(curr))++ret;
			curr = POINTER(curr)->next.load(memory_order_relaxed);
		}
		return ret;
	}

	/**
	 * @brief Add a specified element in to the list's first position.
	 *
	 * @param e
	 * 			The element to be added
	 * @return
	 * 			True for success and false for failed
	 */
	bool push_front(const KeyType& e) {
		return add(e);
	}

	/**
	 * @brief Judge if the list is empty
	 *
	 * @return
	 * 			True is the list is empty, else return false
	 */
	bool empty() {
		return head.load(memory_order_relaxed) == NULL;
	}

	/**
	 * @brief Remove a specified element by value
	 *
	 * @param e
	 * 			The element to be removed
	 * @return
	 * 			If the value exists in the list, remove it and return true. else return false.
	 */
	bool remove(const KeyType& e) {
		return remove(e, &head);
	}

	/**
	 * @brief Judge if the element is in the list
	 *
	 * @param e
	 * 			The element to be searched
	 * @return
	 * 			If the element exists in the list, return true. else return false
	 */
	bool search(const KeyType& e) {
		return search(e, &head);
	}

private:
	/**
	 * @brief Add the specified element into the list
	 *
	 * @param e
	 * 			The element to be added
	 * @return
	 * 			If the element inserted successful return true, else return false
	 */
	bool add(const KeyType& e) {
#ifdef FREELIST
        NodeType<KeyType>* node = mm->newNode();
        node->data = e;
#else
		NodeType<KeyType>* node = new NodeType<KeyType> (e);
#endif
		bool result;
		while (true) {
			/*add to the head of list*/
			/*get the head pointer of the list*/
			NodeType<KeyType>* cur = head.load(memory_order_relaxed);
           
			node->next.store(cur, memory_order_relaxed);
            assert(node->next.load(memory_order_relaxed) == cur);
			/**
			 * CAS operation. set the head of the list to be the new node.
			 * If successful, the insert operation is success and return true.
			 * If failed, it implies that the head pointer must be modified. It must goto the start of this while loop
			 */
			if (head.compare_swap(cur, node)) {
				result = true;
        		break;
			}
		}
		return result;
	}

	/**
	 * @brief Judge if the element is in the list
	 *
	 * @param e
	 * 			The element to be searched
	 * @param start
	 * 			Address of the begin position
	 * @return
	 * 			If the element exists in the list, return true. else return false
	 */
	bool search(const KeyType& e, atomic<NodeType<KeyType>*>* start) {
		FindStateHolder<KeyType> holder = FindStateHolder<KeyType> ();
		bool result = find(e, start, holder);
	    HazardP * hp = mm->getHPRec();	
        retireAll(hp);
		return result;
	}

	/**
	 * @brief Remove a specified element by value
	 *
	 * @param e
	 * 			The element to be removed
	 * @param start
	 * 			address of the begin position
	 * @return
	 * 			If the value exists in the list, remove it and return true. else return false.
	 */
	bool remove(const KeyType& e, atomic<NodeType<KeyType>*>* start) {
		FindStateHolder<KeyType> holder = FindStateHolder<KeyType> ();
		bool result;
        HazardP * hp = mm->getHPRec();

		while (true) {
			if (!find(e, start, holder)) {/* return false if the element is not found in the list*/
				result = false;
				break;
			}
		
            /**
             * CAS operation, mark cur as deleted.
			 * if successful, the thread attempts to remove it at next step.
			 * else, it implies that one or more of three events must have taken place since the snapshot
			 * in Find method was taken. Then goto the start of this while loop
			 */
			if (!holder.cur->next.compare_swap(holder.next, MARK(holder.next))) {
				continue;
			}

		    /**
			 *CAS operation, set the next pointer of prev to the next pointer of cur.
			 *If successful, delete cur by delNode of SMR.
			 *If failed, ,it implies that another thread must have removed the node from the list 
             *after the success of the MARK CAS before.
			 *and a new Find is invoked in order to guarantee that the number of deleted nodes not yet i
             *removed never exceeds the maximum number of concurrent threads operating on the object.
			 */
            assert(!MARKED(holder.next));
			if (holder.prev->compare_swap(holder.cur, holder.next)) {
				mm->delNode(hp, holder.cur);
			} else {
				find(e, start, holder);
			}
			result = true;
			break;
		}
		retireAll(hp);
		return result;
	}

	/**
	 * @brief Assign NULL to all the hazard pointer so that the original
	 * pointer is not marked as "in-use" any more.
	 */
	void retireAll(HazardP * hp) {
		mm->retire(hp, 0);
		mm->retire(hp, 1);
		mm->retire(hp, 2);
	}

	/**
	 * @brief Judge if a specified element is in the list
	 *
	 * @param key
	 * 			A specified element
	 * @param start
	 * 			The address of the begin position
	 * @param holder
	 * 			A node Keep the snapshot of a specified state
	 * @return
	 * 			If the element is existed in the list, return true, else return false
	 */
	bool find(const KeyType& key, atomic<NodeType<KeyType>*>* start, FindStateHolder<KeyType>& holder) {
        HazardP * hp = mm->getHPRec();

		try_again:

		NodeType<KeyType>* next = NULL;
		atomic<NodeType<KeyType>*>* prev = start;

		NodeType<KeyType>* cur = prev->load(memory_order_relaxed);//memory_order_relaxed
        assert(!MARKED(cur));
		mm->employ(hp, 1, cur);

		if (prev->load(memory_order_relaxed) != cur)/* if the holder.pre changed, start over from the beginning */
			goto try_again;

		while (true) {
			if (NULL == cur) {
				/*set value of state holder*/
				holder.isFound = false;
				holder.prev = prev;
				holder.cur = cur;
				holder.next = next;
				return false;
			}

            assert(!MARKED(cur));
			NodeType<KeyType>* markedNext = cur->next.load(memory_order_relaxed);
			bool cmark = MARKED(markedNext);
			next = POINTER(markedNext);

			mm->employ(hp, 0, next);
			if (cur->next.load(memory_order_relaxed) != markedNext) /*If the next node changed, start over from the beginning*/
				goto try_again;

			KeyType cKey = cur->data;
			if (prev->load(memory_order_relaxed) != cur) /*If the holder.pre changed, start over from the beginning*/
				goto try_again;

			if (!cmark) {
				if (cKey == key) {
					/*set value of state holder*/
					holder.isFound = true;
					holder.prev = prev;
					holder.cur = cur;
					holder.next = next;
                    return true;
				}
				prev = &(cur->next);
				//mm->employ(hp, 2, cur);
			} else {/* if the thread encounters a node marked to be deleted, it attempts to remove it from the list*/
				/**
				 *CAS operation, set the next pointer of prew to the next pointer of cur.
				 *If successful, delete holder.cur by delNode of SMR.
				 *If failed, ,it implies that another thread must have removed the node from the list 
                 *after the success of the MARK CAS before.
				 *goto the start position.
				 */
                assert(!MARKED(next));
				if (prev->compare_swap(cur, next)) {
        			mm->delNode(hp, cur);
				} else {
					goto try_again;
				}
			}

			cur = next;
			mm->employ(hp, 1, next);
		}
	}
};

}
#endif /*AMINO_LIST_H*/
