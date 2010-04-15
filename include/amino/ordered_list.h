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
 */

#ifndef AMINO_ORDERDED_LIST_H
#define AMINO_ORDERDED_LIST_H
#include "smr.h"
#include "list.h"

namespace amino {

/**
 * @brief This is a lock-free list, based on:
 *
 * ABA prevention method is based on Maged M. Michael's:
 * Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects
 *
 * This is an implementation of a lock-free linked list data structure. The
 * implementation is according to the paper: High Performance Dynamic Lock-Free Hash Tables and List-Based Sets
 * by Maged M. Michael, 2002. To gain a complete understanding of this data structure, please first read this
 * paper, available at: http://www.research.ibm.com/people/m/michael/spaa-2002.pdf
 *
 * This lock-free linked list is an unbounded thread-safe linked list. A LockFreeList is an appropriate choice
 * when many threads will share access to a common collection. This list does not permit null elements.
 *
 * This is a lock-free implementation intended for highly scalable add, remove and contains which is thread safe.
 * All method related to index is not thread safe. Add() will add the element to the head of the list which is
 * different with the normal list.
 *
 * @author Xiao Jun Dai
 *
 * @tparam KeyType
 * 			The type of the element which is stored in the list
 */

template<typename KeyType> class OrderedList {
public:
	typedef __list_iterator<KeyType> iterator;
	typedef __list_const_iterator<KeyType> const_iterator;

private:
	template<typename T> friend class Set;
	/**
	 * Pointer to header node, initialized by a dummy node.
	 */
	atomic<NodeType<KeyType>*> head;
	/**
	 *a pointer created for initializing of head pointer, for the performance reason
	 */
	NodeType<KeyType>* dummy;
	/*for memory management*/
	SMR<NodeType<KeyType> , NHPOINTER>* mm;
    typedef typename SMR<NodeType<KeyType>, NHPOINTER>::HP_Rec HazardP;
	/* prevent the use of copy constructor and copy assignment*/
	OrderedList(const OrderedList& s) {
	}
	OrderedList& operator=(const OrderedList& st) {
	}

public:
	/**
	 *  @brief constructor. constructs a empty with head is NULL
	 */
	OrderedList() {
		mm = getSMR<NodeType<KeyType> , NHPOINTER> (); /*for memory management, 1 hazard pointer is OK */
		dummy = new NodeType<KeyType> ();
		head.store(dummy->next.load(memory_order_relaxed), memory_order_relaxed);
	}

	/*
	 * @brief Destructor, supposed to be called when its associated data structure is to be deleted
	 */
	~OrderedList() {
		NodeType<KeyType>* tmp = head.load();
		while (tmp != NULL) {/* if the list is not empty. delete the node of it until it is empty*/
			NodeType<KeyType>*tmp_keep = tmp;
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
        NodeType<KeyType>* first = head.load(memory_order_relaxed); 
        assert(!MARKED(first));
        if (first == NULL)
            return false;

        ret = first->data;
        return true;
	}

	/**
	 * @brief Add a specified element in to the list.(the same to the insert method.)
	 *
	 * @param e
	 * 			The element to be added
	 * @return
	 * 			True for success and false for failed
	 */
	bool push_front(const KeyType& e) {
		return add(e, &head);
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
		return add(e, &head);
	}

	/**
	 * @brief Judge if the list is empty
	 *
	 * @return
	 * 			True is the list is empty, else return false
	 */
	bool empty() {
		return head.load() == NULL;
	}

	iterator begin() {
		return head.load(memory_order_relaxed);
	}

	iterator end() {
		return NULL;
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
			 * CAS operation, mark holder.cur as deleted.
			 * if successful, the thread attempts to remove it next step.
			 * else, it implies that one or more of three events must have taken place since the snapshot
			 * in Find method was taken. Then goto the start of this while loop
			 */
            assert(!MARKED(holder.cur));
			if (!holder.cur->next.compare_swap(holder.next, MARK(holder.next))) {
				continue;
			}
			/**
			 *CAS operation, set the next pointer of prew to the next pointer of cur.
			 *If successful, delete holder.cur by delNode of SMR.
			 *If failed, ,it implies that another thread must have removed the node from the list 
             *after the success of the MARK CAS before.
			 *and a new Find is invoked in order to guarantee that the number of deleted nodes i not yet
             *removed never exceeds the maximum i number of concurrent threads operating on the object
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
	 * @brief Judge if the element is in the list
	 *
	 * @param e
	 * 			The element to be searched
	 * @return
	 * 			If the element exists in the list, return true. else return false
	 */
	bool search(const KeyType& e) {
		FindStateHolder<KeyType> holder = FindStateHolder<KeyType> ();
		bool result = find(e, &head, holder);
        HazardP * hp = mm->getHPRec();
		retireAll(hp);
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

private:
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
	 * @brief Add the specified element into the list
	 *
	 * @param e
	 * 			The element to be added
	 * @param start
	 * 			The address of the begin position
	 *
	 * @return
	 * 			If the element inserted successful return true, else return false
	 */
	bool add(const KeyType& e, atomic<NodeType<KeyType>*>* start) {
#ifdef FREELIST	
    	NodeType<KeyType>* node = mm->newNode();
        node->data = e;
#else	
        NodeType<KeyType>* node = new NodeType<KeyType> (e);
#endif	
    	FindStateHolder<KeyType> holder = FindStateHolder<KeyType> ();
		bool result;
        HazardP * hp = mm->getHPRec();
		while (true) {
			if (find(e, start, holder)) {/* if the element is existed in the list. do nothing and return false*/
				delete node;
				result = false;
				break;
			}
			node->next.store(POINTER(holder.cur), memory_order_relaxed);/* set the next pointer of new node to holder.cur */

			/**
			 * CAS operation. set the next pointer of prev to be the new node.
			 * If successful, the insert operation is success and return true.
			 * If failed, it implies that one or more of three events must have taken place since the snapshot in Find method
			 * was taken. then goto the start of this while loop
			 */
			if (holder.prev->compare_swap(holder.cur, POINTER(node))) {
				result = true;
				break;
			}
		}

		retireAll(hp);
		return result;
	}

	/**
	 * @brief add function for lock-free set.
	 *
	 * @param e
	 * 			The element to be added
	 * @param start
	 * 			The address of the begin position
	 * @return
	 * 			If the insertion successful, return the address of the new node, else return the address of existed node
	 */
	NodeType<KeyType>* add_returnAddress(const KeyType& e, atomic<NodeType<KeyType>*>* start) {
#ifdef FREELIST 
        NodeType<KeyType>* node = mm->newNode();
        node->data = e;
#else
		NodeType<KeyType>* node = new NodeType<KeyType> (e);
#endif
		assert(!MARKED(node));
        FindStateHolder<KeyType> holder = FindStateHolder<KeyType> ();
		NodeType<KeyType>* result;
        HazardP * hp = mm->getHPRec();

		while (true) {
			if (find(e, start, holder)) {/* if the element is existed in the list. do nothing and return false*/
				delete node;
				result = holder.cur;
				break;
			}
            assert(!MARKED(holder.cur));
			node->next.store(holder.cur, memory_order_relaxed);/* set the next pointer of new node to holder.cur */
			//FIXME need POINTER(holder.prev) and POINTER(holder.cur) here?

			/**
			 * CAS operation. set the next pointer of prev to be the new node.
			 * If successful, the insert operation is success and return true.
			 * If failed, it implies that one or more of three events must have taken place since the snapshot in Find method
			 * was taken. then goto the start of this while loop
			 */
			if (holder.prev->compare_swap(holder.cur, node)) {
				result = node;
				assert(result != NULL);
				break;
			}
		}

		retireAll(hp);
		return result;
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
	bool find(const KeyType& key, atomic<NodeType<KeyType>*>* start,FindStateHolder<KeyType>& holder) {
		HazardP * hp = mm->getHPRec();

        try_again:
		NodeType<KeyType>* next = NULL;
		atomic<NodeType<KeyType>*>* prev = start;
        
        NodeType<KeyType>* cur = prev->load(memory_order_relaxed);
        assert(!MARKED(cur));

		mm->employ(hp, 1, cur);
		if (prev->load(memory_order_relaxed) != cur)
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

			NodeType<KeyType>* markedNext = cur->next.load(memory_order_relaxed);
			bool cmark = MARKED(markedNext);
			next = POINTER(markedNext);

			mm->employ(hp, 0, next);

			if (cur->next.load(memory_order_relaxed) != markedNext)
				goto try_again;

			KeyType cKey = cur->data;

			if (prev->load(memory_order_relaxed) != cur)
				goto try_again;

			if (!cmark) {
				if (cKey >= key) {/* find the lowest key in the list that is greater than or equal the input key*/
					/*set value of state holder*/
					holder.isFound = (cKey == key);
					holder.prev = prev;
					holder.cur = cur;
					holder.next = next;
					return holder.isFound;
				}
				prev = &(cur->next);
				//mm->employ(hp, 2, cur);
			} else {
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

#endif /*AMINO_ORDERDED_LIST_H*/
