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
/*
 * lockfree_priorityqueue.h
 *
 *  Created on: Sep 11, 2008
 *      Author: daixj
 */

#ifndef LOCKFREE_PRIORITYQUEUE_H_
#define LOCKFREE_PRIORITYQUEUE_H_

#include <amino/smr.h>

#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>

namespace amino {

using namespace internal;

#define READ_NODE(node) (IS_MARKED(node) ? NULL : (node))

/* Macros for set and clear the last bit of memory address. The last bit is used
 * as marker */
#define GET_UNMARKED(p) ((PQNode<E>*)(((long)(p))&(~3)))
#define GET_UNMARKED_VALUE(p) ((Value<E>*)(((long)(p))&(~3)))
/* set the last bit of pointers */
#define GET_MARKED(p) ((PQNode<E>*)(((long)(p))|(1)))
/* set the last bit of pointers */
#define GET_MARKED_VALUE(p) ((Value<E>*)(((long)(p))|(1)))
/* get the last bit of pointers */
#define IS_MARKED(p) (((long)(p))&(1))

/*max level of next pointers in the node*/
#define MAXLEVEL 10

/*
 * @brief Wrapper class for value in the node. Use it since value is need to be marked.
 *
 * @tparam V
 *            type of value in the class
 *
 */
template<typename V> struct Value { // immutable
	V v;

	Value() {
	}

	Value(const V& value) :
		v(value) {
	}
};

/*
 * @brief Node in the priority queue
 *
 * @tparam E
 *            type of value in the priority queue
 *
 */
template<typename E> struct PQNode {
	int level;

	/*
	 * As the nodes, which are used in the lock-free memory management
	 * scheme, will be reused for the same purpose when re-allocated
	 * again after being reclaimed, the individual fields of the nodes
	 * that are not being reclaimed, the individual fields of the nodes
	 * that are not part of the memory management scheme will be intact.
	 * the validLevel field can therefore be used for indicating if the
	 * current node can be used for possibly traversing further on a
	 * certain level. A value of 0 indicates that this node can not be
	 * used for traversing at all, as it is possibly reclaimed or not yet
	 * inserted. As the validLevel field is only set to 0 directly before
	 * reclamation, a positive value indicates that the node is allocated.
	 * A value of n+1 indicated that this node has been inserted up to level n.
	 */
	int validLevel;
	/*
	 * Key in the node
	 */
	E key;
	/*
	 * Value in the node
	 */
	atomic<Value<E>*> value;
	/*
	 * Pointer to previous node
	 */
	PQNode<E>* prev;
	/*
	 * Array of pointers to next nodes
	 */
	atomic<PQNode<E>*> next[MAXLEVEL];

	PQNode() :
		level(0), validLevel(-1), key(), prev(NULL) {
		value.store(NULL);
		for (int i = 0; i < MAXLEVEL; ++i) {
			next[i].store(NULL, memory_order_relaxed);
		}
	}

	PQNode(const E& v) :
		level(0), validLevel(-1), key(v), prev(NULL) {
		value.store(new Value<E> (v), memory_order_relaxed);
		for (int i = 0; i < MAXLEVEL; ++i) {
			next[i].store(NULL, memory_order_relaxed);
		}
	}

	PQNode(int l, const E& v) :
		level(l), validLevel(-1), key(v), prev(NULL) {
		value.store(new Value<E> (v), memory_order_relaxed);
		for (int i = 0; i < MAXLEVEL; ++i) {
			next[i].store(NULL, memory_order_relaxed);
		}
	}

	~PQNode() {
		delete GET_UNMARKED_VALUE(value.load(memory_order_relaxed));
	}
};

/**
 * @brief Factory method to return instance of SMR<T, K>.
 *
 * @tparam Type of node stored inside SMR.
 * @tparam Number of hazard pointers for one thread.
 *
 * @return A SMR instance with the specified template parameter.
 */
template<typename E>
PQNode<E>* getINVALID() {
	static PQNode<E> invalid;
	return &invalid;
}

/**
 * @brief This is an implementation of a lock-free priority queue data structure. The
 * implementation is according to the paper Fast and Lock-Free Concurrent Priority
 * Queues for Multi-Thread Systems by Hakan Sundell and Philippas Tsigas, 2003.
 * To gain a complete understanding of this data structure, please first read this
 * paper, available at: http://portal.acm.org/citation.cfm?id=1073770
 *<p>
 * Priority queues are fundamental data structures. From the operating system level to
 * the user application level, they are frequently used as basic components.
 * A priority queue supports two operations, the Insert and the DeleteMin operation.
 *<p>
 * Inspired by Lotan and Shavit, the algorithm is based on the randomized Skiplist data
 * structure, but in contrast to it is lock-free. The probabilistic time complexity
 * of O(logN) where N is the maximum number of elements in the list.
 *<p>
 * The following operations are thread-safe and scalable: peek, insert, deleteMin, enqueue,
 *  dequeue and empty.
 *<p>
 * The following operations are not thread-safe: size
 *
 * @author Xiao Jun Dai
 *
 * @tparam E
 *            type of element in the priority queue
 */
template<typename E> class LockFreePriorityQueue {
	/*
	 * head pointer
	 * Address of head pointer would be compared, so use pointer here.
	 */
	PQNode<E>* head;
	/*
	 * tail pointer
	 * Address of tail pointer would be compared, so use pointer here.
	 */
	PQNode<E>* tail;

	/*
	 * Dummy node for invalid value.
	 * Address of pointer would be compared, so use pointer here.
	 */
	PQNode<E>* INVALID;

	SMR<PQNode<E> , MAXLEVEL>* mm; // MAXLEVEL for next pointer on every level
	typedef	typename SMR<PQNode<E>, MAXLEVEL>::HP_Rec HazardP;

	int randomSeed;


public:
	LockFreePriorityQueue() {
		mm = getSMR<PQNode<E>, MAXLEVEL> ();

		head = new PQNode<E> ();
		head->validLevel = MAXLEVEL - 1;

		tail = new PQNode<E> ();
		tail->validLevel = MAXLEVEL - 1;

		INVALID = getINVALID<E>();

		/*
		 * Set all next pointers of head to tail
		 */
		for (int i = 0; i < MAXLEVEL; ++i) {
			head->next[i].store(tail, memory_order_relaxed);
		}

		/*
		 * Initialize random number generator for generating random level.
		 */
		srand(time(NULL));

		randomSeed = rand() | 0x0100; // ensure nonzero

	}

	~LockFreePriorityQueue() {
		/*
		 * Destructure all nodes left in the queue
		 */
		PQNode<E> *curr = head->next[0].load(memory_order_relaxed);
		PQNode<E> *tmp = NULL;
		while (tail != curr) {
			tmp = curr;
			curr = curr->next[0].load(memory_order_relaxed);
			delete tmp;
		}

		delete head;
		delete tail;
	}

	/**
	 * @brief The priority queue is empty?
	 * @return true if queue is empty, otherwise false
	 */
	bool empty() {
		return head->next[0].load(memory_order_relaxed) == tail;
	}

	/*
	 * @brief The size of the queue. Compute on the fly.
	 *
	 * @return the size of the queue. It can be inaccurate if parallel
	 * modification exists.
	 */
	int size() {
		int size = 0;
		for (PQNode<E>* itr = head->next[0].load(memory_order_relaxed); itr
				!= tail; itr = itr->next[0].load(memory_order_relaxed)) {
			++size;
		}
		return size;
	}

	/*
	 * @brief Return the minimum value in the queue. Return true if succeed, otherwise false
	 * when the queue is empty. It will not delete the minimum value.
	 *
	 * @param top set to the minimum value if return true, otherwise invalid.
	 *
	 * @return true if the queue is not empty and return the value successfully,
	 * otherwise false
	 */
	bool peek(E& top) {
		top = head->next[0].load(memory_order_relaxed)->value.load(
				memory_order_relaxed)->v;
		if (top == tail) {
			return false;
		} else {
			return true;
		}
	}

	/*
	 * @brief Insert value in the priority queue.
	 *
	 * @param value value inserted to the queue
	 * @return true if succeed, otherwise false
	 */
	bool enqueue(const E& value) {
		return insert(value);
	}

	/*
	 * @brief Return the minimum value in the queue. Return true if succeed, otherwise false
	 * when the queue is empty. The value of top is set to the minimum value if return
	 * true, otherwise invalid. It will delete the minimum value.
	 *
	 * @param result set to the minimum value if return true, otherwise invalid.
	 * @return true if the queue is not empty and return the value successfully,
	 * otherwise false
	 */
	bool dequeue(E& result) {
		return deleteMin(result);
	}

	/*
	 * @brief Insert value in the priority queue.
	 *
	 * @param value value inserted to the queue
	 * @return true if succeed, otherwise false
	 */
	bool insert(const E& value) {
		E key = value;
		int curLevel = randomLevel();
		PQNode<E>* savedNodes[curLevel];

		HazardP * hp = mm->getHPRec();
#ifdef FREELIST
		PQNode<E>* newNode = mm->newNode(hp); /*Allocate a new node*/
		newNode->level = curLevel;
		newNode->key = value;
		newNode->value.store(new Value<E> (value), memory_order_relaxed);
#else
		PQNode<E>* newNode = new PQNode<E> (curLevel, value);/*Allocate a new node*/
#endif
		// Left as a reminder
		//		newNode is local, doesn't need to protected by SMR

	try_again:
		PQNode<E>* node1 = head;
		mm->employ(hp,0,head);
		if(node1 != head) {
			goto try_again;
		}

		PQNode<E>* node2;
		Value<E>* value2;
		/*
		 * search phase to find the node after which the new node(newNode) should
		 * be inserted. This search phase starts from the head node at highest
		 * level and travels down to the lowest level until the correct node
		 * is found(node1). when going down one level, the last node traversed
		 * on that level is remembered(savedNodes) for later use (this is where
		 * we should insert the new node at that level).
		 */
		for (int i = MAXLEVEL - 1; i >= 1; --i) {
			node2 = scanKey(node1, i, key);
			mm->retire(hp,node2);
			if (i < curLevel) {
				savedNodes[i] = node1;

				mm->employ(hp,i,node1);
				if(node1 != savedNodes[i]) {
					goto try_again;
				}
			}
		}

		/*
		 * Loop will terminate until replace the old value with the same key
		 * or insert the new value successfully
		 */
		while (true) {
			node2 = scanKey(node1, 0, key);
			value2 = node2->value.load(memory_order_relaxed);
			/*
			 * now it is possible that there already exists a node with
			 * the same priority as of the new node, the value of the old node(node2)
			 * is changed atomically with a CAS.
			 */
			if ((node2 != tail) && (!IS_MARKED(value2) && (node2->key == key))) {
				if (node2->value.compare_swap(value2, new Value<E> (value))) {
					cout << "found same value: " << value2->v << ":" << value
					<< endl;
					mm->retire(hp,node1);
					mm->retire(hp,node2);
					for (int i = 1; i < curLevel; ++i) {
						mm->retire(hp,savedNodes[i]);
					}
					delete value2;
					mm->delNode(hp,newNode);
					cout << "found same value" << endl;
					return true;
				} else {
					mm->retire(hp,node2);
					continue;
				}
			}
			/*
			 * otherwise, it starts trying to insert the new node starting with
			 * the lowest level and increasing up to the level of the new node.
			 * the next pointers of the nodes (to become previous) are changed
			 * atomically with a CAS.
			 */
			newNode->next[0].store(node2, memory_order_relaxed);
			mm->retire(hp,node2);
			if (node1->next[0].compare_swap(node2, newNode)) {
				mm->retire(hp,node1);
				break;
			}
			//TODO add backoff
		}

		/*
		 * Set the next pointers of new node one by one
		 */
		for (int i = 1; i < curLevel; ++i) {
			newNode->validLevel = i;
			node1 = savedNodes[i];
			while (true) {
				node2 = scanKey(node1, i, key);
				newNode->next[i].store(node2);
				mm->retire(hp,node2);
				/*
				 * After the new node has been inserted at the lowest level,
				 * it is possible that it is deleted by a concurrent deleteMin
				 * operation before it has been inserted at all levels.
				 */
				if (IS_MARKED(newNode->value.load(memory_order_relaxed))
						|| node1->next[i].compare_swap(node2, newNode)) {
					mm->retire(hp,node1);
					break;
				}
				//TODO add backoff
			}
		}

		newNode->validLevel = curLevel;
		if (IS_MARKED(newNode->value.load(memory_order_relaxed))) {
			newNode = helpDelete(newNode, 0);
		}
		// newNode is local variable, no need to release
		return true;
	}

	/*
	 * @brief Return the minimum value in the queue. Return true if succeed, otherwise false
	 * when the queue is empty. The value of top is set to the minimum value if return
	 * true, otherwise invalid. It will delete the minimum value.
	 *
	 * @param result set to the minimum value if return true, otherwise invalid.
	 * @return true if the queue is not empty and return the value successfully,
	 * otherwise false
	 */
	bool deleteMin(E& result) {
		PQNode<E>* node1;
		PQNode<E>* node2;
		PQNode<E>* last;
		Value<E>* value;

		HazardP * hp = mm->getHPRec();
		/*
		 * start from the head node and find the first node (node1) in the list
		 * that does not have its deletion mark on the value set
		 */
	try_again:

		PQNode<E>* prev = head;
		mm->employ(hp,0,head);
		if(prev != head) {
			goto try_again;
		}

		while (true) {
			node1 = readNext(prev, 0);
			if (node1 == tail) {
				mm->retire(hp,prev);
				mm->retire(hp,node1);
				return false;
			}
		retry:
			value = node1->value.load(memory_order_relaxed);
			result = value->v;
			if (!IS_MARKED(value)) {
				/*
				 * it tries to set this deletion mark using CAS primitive.
				 * If it succeeds it also writes a valid pointer to the prev
				 * field of the node. this prev field is necessary in order to
				 * increase the performance of concurrent helpDelete operation,
				 * these operations otherwise would have to search for the previous
				 * node in order to complete the deletion.
				 */
				if (node1->value.compare_swap(value, GET_MARKED_VALUE(value))) {
					node1->prev = prev;
					break;
				} else {
					goto retry;
				}
			} else if (IS_MARKED(value)) {
				node1 = helpDelete(node1, 0);
			}
			mm->retire(hp,prev);
			prev = node1;
		}
		/*
		 * the next step is to mark the deletion bits of the next pointer
		 * in the nodes, starting with the lowest level and going upwards,
		 * using the CAS primitive in each step
		 */
		assert(node1 != tail && node1 != head);
		for (int i = 0; i < node1->level; ++i) {
			do {
				node2 = node1->next[i].load(memory_order_relaxed);
			}while (!IS_MARKED(node2) && !node1->next[i].compare_swap(node2, GET_MARKED(node2)));
		}
		prev = head;
		mm->employ(hp,0,head);
		if(prev != head) {
			goto try_again;
		}
		/*
		 * starts the actual deletion by changing the next pointers of the
		 * previous node(prev), starting at the highest level and continuing
		 * downwards. the reason for doing the deletion in decreasing order
		 * of levels, is that concurrent search operations also start at the
		 * highest level and processed downwards, in this way the concurrent search
		 * operations will sooner avoid traversing this node. the procedure performed
		 * by the deletemin operation in order to change each next pointer of
		 * the previous node, is to first search for the previous node and then
		 * perform the CAS primitive until it succeeds.
		 */
		for (int i = node1->level - 1; i >= 0; --i) {
			while (true) {
				assert(node1 != tail && node1 != head);
				if (GET_UNMARKED(node1->next[i].load(memory_order_relaxed)) == INVALID) {
					break;
				}
				last = scanKey(prev, i, node1->key);
				mm->retire(hp,last);
				if (last != node1 || GET_UNMARKED(node1->next[i].load(memory_order_relaxed)) == INVALID) {
					break;
				}
				if (prev->next[i].compare_swap(node1, GET_UNMARKED(
										node1->next[i].load(memory_order_relaxed)))) {
					node1->next[i].store(INVALID, memory_order_relaxed);
					break;
				}
				if (GET_UNMARKED(node1->next[i].load(memory_order_relaxed)) == INVALID) {
					break;
				}
				//TODO add backoff
			}
		}
		mm->retire(hp,prev);
		/* delete the node */
		mm->delNode(node1);
		return true;
	}

#ifdef DEBUG
	/*
	 * @brief Dump the priority queue.
	 */
	void dumpQueue() {
		cout << "dumping queue...................." << endl;
		cout << "head:" << head << "->";
		for (PQNode<E>* itr = head->next[0].load(memory_order_relaxed); itr
				!= tail; itr = itr->next[0].load(memory_order_relaxed)) {
			cout << GET_UNMARKED(itr) << "->";
		}
		cout << tail << ":tail" << endl;
	}
#endif

private:
	/**
	 * @brief traverse to the next node on the given level while helping any deleted
	 * nodes in between to finish the deletion.
	 *
	 * @param node
	 * @param curLevel current level
	 */
	PQNode<E>* readNext(PQNode<E>*& node, int curLevel) {
		PQNode<E>* nextNode;
		PQNode<E>* tmpNode;

		assert(node != tail);
		if (IS_MARKED(node->value.load(memory_order_relaxed))) {
			node = helpDelete(node, curLevel);
		}
		tmpNode = node->next[curLevel].load(memory_order_relaxed);
		nextNode = READ_NODE(tmpNode);
		while (NULL == nextNode) {
			node = helpDelete(node, curLevel);
			tmpNode = node->next[curLevel].load(memory_order_relaxed);
			nextNode = READ_NODE(tmpNode);
		}
		assert(!IS_MARKED(nextNode));
		return nextNode;
	}

	/**
	 * @brief traverse in several steps through the next pointers at the current level
	 * until it fins a node that has the same or higher key than the given key.
	 */
	PQNode<E>* scanKey(PQNode<E>*& node, int curLevel, const E& expectedKey) {
		HazardP * hp = mm->getHPRec();
		PQNode<E>* nextNode = readNext(node, curLevel);
		while ((nextNode != tail) && ((nextNode == head) || nextNode->key
						< expectedKey)) {
			mm->retire(hp,node);
			node = nextNode;
			nextNode = readNext(node, curLevel);
		}
		return nextNode;
	}

	/*
	 * @brief it tries to fulfill the deletion on the current level and returns when it is completed.
	 */
	PQNode<E>* helpDelete(PQNode<E>* node, int curLevel) {
		HazardP * hp = mm->getHPRec();

		PQNode<E>* nextNode;
		PQNode<E>* last;
		/*
		 * it starts with setting the deletion mark on all next pointers
		 * in case they have not been set.
		 */
		for (int i = curLevel; i < node->level; ++i) {
			do {
				nextNode = node->next[i].load(memory_order_relaxed);
			}while (!IS_MARKED(nextNode) && !node->next[i].compare_swap(nextNode, GET_MARKED(
									nextNode)));
		}
		/*
		 * it checks if the node given in the prev field is valid for deletion of
		 * on the current level
		 */
		PQNode<E>* prev = node->prev;
		if (NULL == prev || (curLevel >= prev->validLevel)) {
			/*
			 * otherwise it searches for the correct node (prev)
			 */
			try_again:
			prev = head;
			mm->employ(hp,0,head);
			if(prev != head) {
				goto try_again;
			}

			for (int i = MAXLEVEL - 1; i >= curLevel; --i) {
				nextNode = scanKey(prev, i, node->key);
				mm->retire(hp, nextNode);
			}
		}
		// prev is local variable, no need to copy_node
		/* the actual deletion of this node on the current level takes place.
		 * this operation might execute concurrently with the corresponding
		 * deleteMin operation, and therefore both operations synchronize with
		 * each other in order to avoid executing sub-operations that have already
		 * been performed.
		 */
		while (true) {
			assert(node != tail && node != head);
			if (GET_UNMARKED(node->next[curLevel].load(memory_order_relaxed)) == INVALID) {
				break;
			}
			last = scanKey(prev, curLevel, node->key);
			mm->retire(hp,last);
			if (last != node || GET_UNMARKED(node->next[curLevel].load(memory_order_relaxed)) == INVALID) {
				break;
			}
			if (prev->next[curLevel].compare_swap(node, GET_UNMARKED(
									node->next[curLevel].load(memory_order_relaxed)))) {
				node->next[curLevel].store(INVALID);
				break;
			}
			if (GET_UNMARKED(node->next[curLevel].load(memory_order_relaxed)) == INVALID) {
				break;
			}
			//TODO add backoff

}
		mm->retire(hp,node);
		return prev;
	}

   /*
    * @brief Returns a random level for inserting a new node.
    *
    * This algorithm of the generators is described in George
    * Marsaglia's "Xorshift RNGs" paper.
    *
    */
	int randomLevel() {
       int x = randomSeed;
        x ^= x << 13;
        x ^= x >> 17;
        randomSeed = x ^= x << 5;
        int level = 1;
        while ((((x >>= 1) & 1) != 0) && (level < MAXLEVEL - 1)) {
        	++level;
        }

        return level;
	}
};

}

#endif /* LOCKFREE_PRIORITYQUEUE_H_ */
