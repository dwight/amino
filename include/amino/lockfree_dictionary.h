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
 * lockfree_dictionary.h 
 *
 *  Created on: Sep 9, 2008
 *      Author: daixj
 */

#ifndef LOCKFREE_DICTIONARY_H_
#define LOCKFREE_DICTIONARY_H_

#include <amino/smr.h>

namespace amino {

using namespace internal;

#define READ_NODE(node) (IS_MARKED(node) ? NULL : (node))

/* Macros for set and clear the last bit of memory address. The last bit is used
 * as marker */
#define GET_UNMARKED(p) ((DictNode<K,V>*)(((long)(p))&(~3)))
#define GET_UNMARKED_VALUE(p) ((Value<V>*)(((long)(p))&(~3)))
/* set the last bit of pointers */
#define GET_MARKED(p) ((DictNode<K,V>*)(((long)(p))|(1)))
/* set the last bit of pointers */
#define GET_MARKED_VALUE(p) ((Value<V>*)(((long)(p))|(1)))
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
template<typename E> struct Value {// immutable
	E v;

	Value() {
	}

	Value(const E& value) :
		v(value) {
	}
};

/*
 * @brief Node in the dictionary
 *
 * @tparam K
 *            type of key in the dictionary
 *
 * @tparam V
 *            type of value in the dictionary
 *
 */
template<typename K, typename V> struct DictNode {
    // REVIEW: what's the difference between level and valid level?
	int level;
	/* REVIEW: difficult to understand. Sentence is too long?
     *
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
    // REVIEW: what's version?
	int version;
	/*
	 * Key in the node
	 */
	K key;
	/*
	 * Value in the node
	 */
	atomic<Value<V>*> value;
	/*
	 * Pointer to previous node
	 */
	DictNode<K, V>* prev;
	/*
	 * Array of pointers to next nodes
	 */
	atomic<DictNode<K, V>*> next[MAXLEVEL];

	DictNode() :
		level(-1), validLevel(0), version(0), key(), prev(NULL) 
    {
            //REVIEW why need a strong barrier?
		value.store(NULL);
		for (int i = 0; i < MAXLEVEL; ++i) {
			next[i].store(NULL, memory_order_relaxed);
		}
	}

    //REVIEW: api doc? argment name is diffciult to understand
	DictNode(int l, K k, V v) :
		level(-1), validLevel(0), version(0), key(k), prev(NULL) {
		value.store(new Value<V> (v), memory_order_relaxed);
		for (int i = 0; i < MAXLEVEL; ++i) {
			next[i].store(NULL, memory_order_relaxed);
		}
	}

    ~DictNode() {
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
template<typename K, typename V>
DictNode<K,V>* getINVALID() {
	static DictNode<K,V> invalid;
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
 * @tparam K
 *            type of key in the dictionary
 *
 * @tparam V
 *            type of value in the dictionary
 */
template<typename K, typename V> class LockFreeDictionary {
private:
	/*
	 * Head pointer
	 * Address of head pointer would be compared, so use pointer here.
	 */
	DictNode<K, V>* head;
	/*
	 * Tail pointer
	 * Address of tail pointer would be compared, so use pointer here.
	 */
	DictNode<K, V>* tail;
	/*
	 * Dummy node for invalid value.
	 * Address of pointer would be compared, so use pointer here.
	 */
	DictNode<K, V>* INVALID;

    SMR<DictNode<K, V>, MAXLEVEL>* mm; // MAXLEVEL for next pointer on every level
   	typedef typename SMR<DictNode<K, V>, MAXLEVEL>::HP_Rec HazardP;

   	/*
   	 * Seed for random level generator.
   	 */
   	int randomSeed;

public:
	LockFreeDictionary() {
        mm = getSMR<DictNode<K, V>, MAXLEVEL> ();

        //REIVEW: can be a class field instead of a pointer?
		head = new DictNode<K, V> ();
		head->validLevel = MAXLEVEL - 1;

		tail = new DictNode<K, V> ();
		tail->validLevel = MAXLEVEL - 1;

		INVALID = getINVALID<K,V>();

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

    ~LockFreeDictionary() {
		/*
		 * Destructure all nodes left in the dictionary
		 */
		DictNode<K,V> *curr = head->next[0].load(memory_order_relaxed);
        DictNode<K,V> *tmp = NULL;
		while (tail != curr) {
            tmp = curr;
            curr = curr->next[0].load(memory_order_relaxed);
            delete tmp;
		}

        delete head;
        delete tail;
    }

	/**
	 * @brief Is the dictionary empty?
	 * @return true if dictionary is empty, otherwise false
	 */
	bool empty() {
		return head->next[0].load(memory_order_relaxed) == tail;
	}

	/*
     * REVIEW: the result is inaccurate if current modification?
	 * @brief The size of the dictionary. Compute on the fly.
	 *
	 * @return the size of the dictionary. It can be inaccurate if parallel
	 * modification exists.
	 */
	int size() {
		int size = 0;
		for (DictNode<K,V>* itr = head->next[0].load(memory_order_relaxed); itr
				!= tail; itr = itr->next[0].load(memory_order_relaxed)) {
			++size;
		}
		return size;
	}

	/*
	 * @brief Insert value in the dictionary.
	 *
	 * @param key key inserted to the dictionary
	 * @param value value inserted to the dictionary
     * REVIEW: when will return true? If duplicate, will return false?
	 * @return true if succeed, otherwise false
	 */
	bool insert(const K& key, const V& value) {
		DictNode<K, V>* node1;
		DictNode<K, V>* node2;
        // REIVEW: where is value1?
		Value<V>* value2;
		int curLevel = randomLevel();

		HazardP * hp = mm->getHPRec();
#ifdef FREELIST
        DictNode<K, V>* newNode = mm->newNode(hp); /*Allocate a new node*/
        newNode->level = curLevel;
        newNode->key = key;
        newNode->value.store(new Value<V> (value), memory_order_relaxed);
#else
        DictNode<K, V>* newNode = new DictNode<K, V> (curLevel, key, value);/*Allocate a new node*/
#endif
		//		newNode is local, doesn't need to protected by SMR

		/* search phase to find the node after which the new node (newNode)
		 * should be inserted. This search phase starts from the head node
		 * at the head node at the highest level and traverse down to the
		 * lowest level until the correct node is found (node1). When going
		 * down one level, the last node traversed on that level is remembered
		 * (savedNodes) for later use (this is where we should insert the new
		 * node at that level).
		 */
		DictNode<K, V>* savedNodes[MAXLEVEL + 1];
		savedNodes[MAXLEVEL] = head;
		for (int i = MAXLEVEL - 1; i >= 0; --i) {
			savedNodes[i] = searchLevel(savedNodes[i + 1], i, key);
			if ((i < MAXLEVEL - 1) && (i >= curLevel - 1)) {
                mm->retire(hp,savedNodes[i + 1]);
			}
		}
		node1 = savedNodes[0];

		/*
		 * Loop will terminate until replace the old value with the same key
		 * or insert the new value successfully
		 */
		while (true) {
			node2 = scanKey(node1, 0, key);
			value2 = node2->value.load(memory_order_relaxed);
			/* now it is possible that there already exists a node with the same key as of
			 * the new node, the value of the old node (node2) is changed atomically with a CAS.
			 *
			 * all keys on nodes are greater than key on head and less than key on tail
			 */
			if (!IS_MARKED(value2) && (node2->value.load(memory_order_relaxed) != NULL
					&& node2->key == key)) {
				if (node2->value.compare_swap(value2, new Value<V> (value))) {
                    mm->retire(hp,node1);
                    mm->retire(hp,node2);
					for (int i = 1; i < curLevel; ++i) {
                        mm->retire(hp,savedNodes[i]);
					}
                    // REVIEW: need SMR?
                    delete value2;
                    mm->delNode(hp,newNode);
					return true;
				} else {
                    mm->retire(hp,node2);
					continue;
				}
			}
			/* otherwise, it starts trying to insert the new node starting with the lowest
			 * level increasing up to the level of the new node. The next pointers of the
			 * (to be previous) nodes are changed atomically with a CAS. */
			newNode->next[0] = node2;
            mm->retire(hp,node2);
			if (node1->next[0].compare_swap(node2, newNode)) {
                mm->retire(hp,node1);
				break;
			}
			//TODO add backoff
		}
		++newNode->version;
		newNode->validLevel = 1;

		/*
		 * Set the next pointers of new node one by one
		 */
		for (int i = 1; i < curLevel; ++i) {
			node1 = savedNodes[i];
			while (true) {
				node2 = scanKey(node1, i, key);
				newNode->next[i] = node2;
                mm->retire(hp,node2);
				/* after the new node has been insered at the lowest level, it is possible that it is
				 * deleted by a concurrent Delete operation before it has been inserted at all levels,
				 */
				if (IS_MARKED(newNode->value.load(memory_order_relaxed))) {
                    mm->retire(hp,node1);
					break;
				}
				if (node1->next[i].compare_swap(node2, newNode)) {
					newNode->validLevel = i + 1;
                    mm->retire(hp,node1);
					break;
				}
				// TODO add backoff
			}
		}

		if (IS_MARKED(newNode->value.load(memory_order_relaxed))) {
			newNode = helpDelete(newNode, 0);
		}
        mm->retire(hp,newNode);
        // newNode is local variable, no need to release
		return true;
	}

	/*
	 * @brief Find the expected key in the dictionary
	 *
	 * @param key expected key to find
	 * @param value return the value with the expected key
	 *
	 * @return true if found the key, otherwise false
	 */
	bool findKey(const K& key, V& value) {
        HazardP * hp = mm->getHPRec();

		/* basically follows the Insert operation */
	try_again:
        DictNode<K, V>* last = head;

   		mm->employ(hp,0,head);
		if(last != head) {
			goto try_again;
		}

		DictNode<K, V>* node1;
		DictNode<K, V>* node2;

		/*
		 * Search key from top level downwards
		 */
		for (int i = MAXLEVEL - 1; i >= 0; --i) {
			node1 = searchLevel(last, i, key);
            mm->retire(hp,last);
			last = node1;
		}

		node2 = scanKey(last, 0, key);
        mm->retire(hp,last);
		Value<V>* result = node2->value.load(memory_order_relaxed);

		/*
		 * Didn't find the key or the key is being deleted.
		 */
		if ((node2->key != key) || (IS_MARKED(result))) {
            mm->retire(hp,node2);
			return false;
		}
		value = result->v;
        mm->retire(hp,node2);
		return true;
	}

	/*
	 * @brief Delete the expected key in the dictionary
	 *
	 * @param key expected key to find
	 * @param value return the value with the expected key
	 *
	 * @return true if found the key, otherwise false
	 */
	bool deleteKey(const K& key, V& value) {
		return _delete(key, false, value);
	}

	/*
	 * @brief Find the expected value in the dictionary
	 *
	 * @param value expected value to find
	 * @param key return the key with the expected value
	 *
	 * @return true if found the value, otherwise false
	 */
	bool findValue(const V& value, K& key) {
		return fDValue(value, false, key);
	}

	/*
	 * @brief Delete the expected value in the dictionary
	 *
	 * @param value expected value to find
	 * @param key return the key with the expected value
	 *
	 * @return true if found the value, otherwise false
	 */
	bool deleteValue(const V& value, K& key) {
		return fDValue(value, true, key);
	}

#ifdef DEBUG
	/*
	 * @brief Dump the priority queue.
	 */
	void dumpQueue() {
		cout << "dumping queue...................." << endl;
		cout << "head:" << head << "->";
		for (DictNode<K, V>* itr = head->next[0].load(memory_order_relaxed); itr
				!= tail; itr = itr->next[0].load(memory_order_relaxed)) {
			cout << GET_UNMARKED(itr)->value.load(memory_order_relaxed)->v << "->";
		}
		cout << tail << ":tail" << endl;
	}
#endif

private:
	/*
	 * @brief Traverse rapidly from an allocated node last and return the node which key
	 * field is the highest key that is lower than the searched key at the current level.
	 *
	 * @param last start node for searching
	 * @param curLevel level to search
	 * @param expectedKey key to search
	 *
	 * @return the node with expected key in the level
	 */
	DictNode<K, V>* searchLevel(DictNode<K, V>*& last, int curLevel,
			const K& expectedKey) {
        HazardP * hp = mm->getHPRec();

    try_again:
		DictNode<K, V>* curNode = last;
		DictNode<K, V>* stop = NULL;
		DictNode<K, V>* nextNode = NULL;

		assert(last != tail);

		while (true) {
			nextNode = GET_UNMARKED(curNode->next[curLevel].load(memory_order_relaxed));
			/*
			 * reach the end of this level
			 */
			if (NULL == nextNode) {
				if (curNode == last) {
					last = helpDelete(last, curLevel);
				}
				curNode = last;
			}
			/*
			 *current key is lower than expected key, continue search
			 */
			else if ((nextNode != head) && (nextNode == tail || nextNode->key
					>= expectedKey)) {
				assert(nextNode != head);
                // REIVEW: position of this comment is not approriate?
                // curNode is local, no need to proteted by SMR

				/*
				 * current key is within the search boundaries. When the node
				 * suitable for returning has been reached, it is checked
				 * that it is allocated and also assured that it hen stays
				 * allocated. If this succeeds the node is returned, otherwise
				 * the traversal restarted at node last. If this fails twice,
				 * the traversal are done using the safe scanKey operations,
				 * as this indicates that the node possibly is inserted at
				 * the current level, but the validLevel field has not yet
				 * been updated. In case the node last is marked for deletion,
				 * it might have been deleted at the current level and thus it
				 * can not be used for traversal. Therefore the node last is
				 * checked if it is marked. If marked, the node last will be
				 * helped fully delete on the current level and last is set to
				 * the previous node.
				 *
				 */
				if ((curNode->validLevel > curLevel || curNode == last
						|| curNode == stop) && ((curNode != tail) && (curNode
						== head || curNode->key < expectedKey))
						&& (last == head || curNode == tail || curNode->key
								>= last->key)) {
					if (curNode->validLevel <= curLevel) {
                        mm->retire(hp,curNode);

                        curNode = last;
                   		mm->employ(hp,0,last);
                		if(curNode != last) {
                			goto try_again;
                		}

						nextNode = scanKey(curNode, curLevel, expectedKey);
                        mm->retire(hp,nextNode);
					}
					return curNode;
				}
                mm->retire(hp,curNode);

				stop = curNode;
				if (IS_MARKED(last->value.load(memory_order_relaxed))) {
					last = helpDelete(last, curLevel);
				}
				curNode = last;
			}
			/*
			 * current key is within the search boundaries
			 */
			else if (last != tail && nextNode != head && (last == head
					|| nextNode == tail || nextNode->key >= last->key)) {
				assert(last != tail);
				curNode = nextNode;
			}
			/*
			 * reach the end of current level but not found a higher key than
			 * expected key
			 */
			else {
				if (IS_MARKED(last->value.load(memory_order_relaxed))) {
					last = helpDelete(last, curLevel);
				}
				curNode = last;
			}
		}
	}

	/*
	 * @brief Find the expected key in the dictionary.
	 *
	 * @param key expected key to search
	 * @param delval true if delete this key, otherwise don't delete
	 * @param value return the value with the key
	 *
	 * @return true if find the key, otherwise false
	 */
	bool _delete(const K& key, bool delval, V& value) {
        HazardP * hp = mm->getHPRec();

		DictNode<K, V>* node1;
		DictNode<K, V>* node2;
		DictNode<K, V>* prev;
		DictNode<K, V>* last;
		DictNode<K, V>* savedNodes[MAXLEVEL + 1];
		Value<V>* tmpValue = NULL;

		/* Start with a search phase to find the first node which key is
		 * equal or higher than the searched key. this search phase starts
		 * from the head node at the highest level and traverse down to the
		 * lowest level until the correct node is found (node1). When going
		 * down one level, the last node traversed on that level is remembered
		 * (savedNodes) for later use (this is the previous node at which the
		 * next pointer should be changed in order to delete the targeted node
		 * at that level.)
		 */
    try_again:
		savedNodes[MAXLEVEL] = head;
		for (int i = MAXLEVEL - 1; i >= 0; --i) {
			savedNodes[i] = searchLevel(savedNodes[i + 1], i, key);
		}
		node1 = scanKey(savedNodes[0], 0, key);

        /* Don't find */
        if (node1 == tail) {
            return false;
        }

		while (true) {
			if (!delval) {
				tmpValue = node1->value.load(memory_order_relaxed);
                value = tmpValue->v;
			}
			/*
			 * node1->value.load(memory_order_relaxed)->v == value is only
			 * called by FdValue
			 */
			if ((node1->value.load(memory_order_relaxed) != NULL && node1->key
					== key) && (!delval || node1->value.load(
					memory_order_relaxed)->v == value) && (!IS_MARKED(tmpValue))) {
				/* if the found node is the correct node, it tries to set the
				 * deletion mark of the value field using CAS primitive, and
				 * if it succeeds it also writes a valid pointer (which
				 * corresponding node will stay allocated until this node gets
				 * reclaimed) to the prev field of the node. This prev field is
				 * necessary in order to increase the performance of concurrent
				 * HelpDelete operations. these otherwise would have to search for
				 * the previous node in order to complete the deletion.*/
				if (node1->value.compare_swap(tmpValue, GET_MARKED_VALUE(tmpValue))) {
                    int index = (node1->level - 1)/2;
					node1->prev = savedNodes[index];

                    mm->employ(hp,0,savedNodes[index]);
                	if(node1->prev != savedNodes[index]) {
                		goto try_again;
                	}

					break;
				} else {
					continue;
				}
			}
            mm->retire(hp,node1);
            /*
             * retire node from SMR.
             */
			for (int i = 0; i < MAXLEVEL; ++i) {
                mm->retire(hp,savedNodes[i]);
			}
			return false;
		}

		/*
		 * The next step is to mark the deletion bits of the next pointers in
		 * the node, starting with the lowest level and going upwards, using
		 * the CAS primitive in each step
		 */
		for (int i = 0; i < node1->level; ++i) {
			do {
				node2 = node1->next[i].load(memory_order_relaxed);
			} while (!IS_MARKED(node2) && !node1->next[i].compare_swap(node2, GET_MARKED(node2)));
		}

		/*
		 * It starts the actual deletion by changing the next pointers of the
		 * previous node (prev), starting at the highest level and continuing
		 * downward. the reason for doing the deletion in decreasing order of
		 * levels, is that concurrent operations that are in the search phase also
		 * start at the highest level and proceed downwards, in this way the
		 * concurrent search operations will sooner avoid traversing this node.
		 * the procedure performed by Delete operation in order to change each
		 * next pointer of the previous nodes, is to first search for the previous
		 * node and then perform the CAS primitive until it succeeds.
		 */
		for (int i = node1->level - 1; i >= 0; --i) {
			prev = savedNodes[i];
			while (true) {

				if (node1->next[i].load(memory_order_relaxed) == INVALID) {
					break;
				}
				last = scanKey(prev, i, node1->key);
                mm->retire(hp,last);
				if ((last != node1) || (node1->next[i].load(
						memory_order_relaxed) == INVALID)) {
					break;
				}
				if (prev->next[i].compare_swap(node1, GET_UNMARKED(node1->next[i].load(memory_order_relaxed)))) {
					node1->next[i].store(INVALID, memory_order_relaxed);
					break;
				}
				if (node1->next[i].load(memory_order_relaxed) == INVALID) {
					break;
				}
				// TODO backoff
			}
            mm->retire(hp,prev);
		}
		for (int i = node1->level; i < MAXLEVEL; ++i) {
            mm->retire(hp,savedNodes[i]);
		}

		mm->delNode(node1);
		return true;
	}

	/*
	 * @brief Traverse from the head node along the lowest level in the skiplist until
	 * a node with the searched value is found. In every traversal step, it has
	 * to be assured that the step is taken from a valid node to a valid node,
	 * both valid at the same time. The validLevel field of the node can be used
	 * to safely verify the validity, unless the node has be reclaimed.
	 *
	 * @param value expected value to search
	 * @param true if delete the value, otherwise don't delete // REVIEW what param?
	 * @param key return the key with the value
	 *
	 * @return true if find the value, otherwise false
	 */
	bool fDValue(const V& value, bool del, K& key) {//REVIEW can't understand this function name :)
        HazardP * hp = mm->getHPRec();
		DictNode<K, V>* node1;
		DictNode<K, V>* node2;
		bool ok;
		int version;
		int version2;
		K key2;

        //REVIEW: what's the meaning of jump and why 16?
		int jump = 16;
        //REIVEW: indent it :)
        try_again:
		DictNode<K, V>* last = head;

        mm->employ(hp,0,head);
       	if(last != head) {
       		goto try_again;
       	}

     next_jump:
        node1 = last;
		K key1 = node1->key;
		int step = 0;

		while (true) {
			ok = false;
			/* The versions field is incremented by the Insert operation, after the node has been
			 * inserted at the lowest level, and directly before the validLevel is set
			 * to indicate validity. By performing two consecutive reads of the version
			 * field with the same contents, and successfully verifying he validity in between
			 * the reads, it can be concluded that the node has stayed valid from the first
			 * read of the version until he successful validity check.
			 */
			version = node1->version;
			node2 = node1->next[0].load(memory_order_relaxed);
			if (!IS_MARKED(node2) && (node2 != NULL)) {
				version2 = node2->version;
				key2 = node2->key;
				assert(node1 != tail);
				assert(node2 != head);
				// node1 and node2 could be head and tail

				if ((node1->key == key1) && (node1->validLevel > 0)
						&& (node1->next[0].load(memory_order_relaxed) == node2
								&& (node1->version == version) && (node2->key
								== key2) && (node2->validLevel > 0)
								&& (node2->version == version2))) {
					ok = true;
				}
			}
			/*
			 * If this fails, it restarts and traverse the safe node last one step
			 * using ReadNext function.
			 */
			if (!ok) {
				node1 = node2 = readNext(last, 0);
				key1 = key2 = node2->key;
				version2 = node2->version;
                mm->retire(hp,last);
				last = node2;
				step = 0;
			}

			/*
			 * this traversal fails until the tail node is reached
			 */
			if (node2 == tail) {
                mm->retire(hp,last);
				return false;
			}
			/*
			 * this traversal is continued until a node with the searched
			 * value is reached
			 */
			if (node2->value.load(memory_order_relaxed)->v == value) {
				if (node2->version == version2) {
					/*
					 * In case the found node should be deleted, the Delete operation
					 * if called for this purpose
					 */
					if (del) {
						// DeleteValue always reach here
						V tmpValue = value;
						bool result = _delete(key2, true, tmpValue);
						if (result && tmpValue == value) {
                            mm->retire(hp,last);
							key = key2;
							return true;
						}
					} else {
						// DeleteKey always reach here
                        mm->retire(hp,last);
						key = key2;
						return true;
					}
				}
			}
			/*
			 * After a certain number (jump) of successful fast step, an attempt
			 * to advance the last node to current position is performed.
			 */
			else if (++step >= jump) {
                //node2 is local variable, no need to be protected by SMR
				/*
				 * If this attempt succeeds, the threshold jump is increased by 1.5 times,
				 * otherwise it is halved.
				 */
				if ((node2->validLevel == 0) || (node2->key != key2)) {
                    mm->retire(hp,node2);
					node2 = readNext(last, 0);
					if (jump >= 4) {
						jump /= 2;
					}
				} else {
					jump += jump / 2;
				}
                mm->retire(hp,last);
				last = node2;
				goto next_jump;
			} else {
				key1 = key2;
				node1 = node2;
			}
		}
	}

	/**
	 * @brief Traverse to the next node on the given level while helping any deleted
	 * nodes in between to finish the deletion.
	 *
	 * TODO the use of the safe ReadNext and ScanKey operations for traversing
	 * the Skiplist will cause the performance to be significantly lower compared
	 * to the sequential case where the next pointers are used directly.
	 *
	 * @param node continuously updated to point to the previous node of the returned node
	 * @param curLevel current level
	 *
	 * @return next node
	 */
	DictNode<K, V>* readNext(DictNode<K, V>*& node, int curLevel) {
		DictNode<K, V>* nextNode;
		DictNode<K, V>* tmpNode;

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

	/*
	 * @brief Traverse in several steps through the next pointers at the current level
	 * until it finds a node that has the same or higher key than the given key.
	 *
	 * @param node continuously updated to point to the previous node of the returned node
	 * @param curLevel current level
	 * @param expectedKey expected key to find
	 *
	 * @return node that has the same or higher key than the given key
	 */
	DictNode<K, V>* scanKey(DictNode<K, V>*& node, int curLevel,
			const K& expectedKey) {
        HazardP * hp = mm->getHPRec();

        DictNode<K, V>* nextNode = readNext(node, curLevel);
		while ((nextNode != tail) && ((nextNode == head) || nextNode->key
				< expectedKey)) {
            mm->retire(hp,node);
			node = nextNode;
			nextNode = readNext(node, curLevel);
		}
		return nextNode;
	}

	/*
     * @brief Traverse to the next node on the given level while helping any
     * deleted nodes in between to finish the deletion. 
     *
     * The algorithm has been designed for pre-emptive as well as full
     * concurrent systems. In order to archieve the lock-free property (that at
     * least one thread is doing progress) on pre-emptive systems. whenever a
     * search operation finds a node that is about to be deleted, it calls the
     * HelpDelete operation and then proceeds searching from the previous node
     * of then proceeds searching from the previous node of the deleted.
	 *
     * This operation might execute concurrently with the corresponding Delete
     * operation as well with other HelpDelete operations, and therefore all
     * operations synchronized with each other in order to avoid executing
     * sub-operations that have already been performed.
	 *
     * In fully concurrent systems though, the helping strategy can downgrade
     * the performance significantly.  Therefore the algorithm, after a number
     * of consecutive failed attempts to help concurrent Delete operations that
     * stops the progress of the current operation, puts the current operation,
     * puts the current operation into back-off mode. When in back-off mode,
     * the thread does nothing for a while, and in this way avoids disturbing the
     * concurrent operations that might otherwise progress slower. The duration
     * of back-offs is proportional to the number of threads and for each
     * consecutive entering of back-off mode during one operation invocation,
     * the duration is increased expononetially.
	 *
     * @param node start node @param curLevel current level
	 *
     * @return first node which is not marked as deleted
	 */
    DictNode<K, V>* helpDelete(DictNode<K, V>* node, int curLevel) {
        assert(node != tail && node != head); HazardP * hp = mm->getHPRec();

		DictNode<K, V>* prevNode;
		DictNode<K, V>* node1;
		DictNode<K, V>* node2;
		DictNode<K, V>* last;

    try_again:
		/*
		 * setting the deletion mark on all next pointers in case they have not been set.
		 * physical deletion of continuous marked nodes on all levels
		 * from the next node of node
		 */
		for (int i = curLevel; i < node->level; ++i) {
			/*
			 * physical deletion of continuous marked nodes on current level
			 * from the next node of node
			 */
			do {
				node2 = node->next[i].load(memory_order_relaxed);
			} while (!IS_MARKED(node2) && (!node->next[i].compare_swap(node2, GET_MARKED(node2))));
		}

		/*
		 * it checks if the node given in the prev field is valid for deletion on the current
		 * level, otherwise it starts the search at the head node.
		 * increase the reference counter for node
		 */
		prevNode = node->prev;
		if ((prevNode == NULL) || (curLevel >= prevNode->validLevel)) {
			prevNode = head;

            mm->employ(hp,0,head);
            if(prevNode != head) {
                goto try_again;
            }
		}
        // REVIEW: adjust position of following comment?
        // no need to protecteed by SMR, prevNode is local variable

		/*
		 * repeat until the node is deleted at the current level.
		 * remove the current node. it is also marked.
		 */
		while (true) {
			if (INVALID == node->next[curLevel].load(memory_order_relaxed)) {
				break;
			}
			/* it searches for the correct node (prev). */
			for (int i = prevNode->validLevel - 1; i >= curLevel; --i) {
				node1 = searchLevel(prevNode, i, node->key);
                mm->retire(hp,prevNode);
				prevNode = node1;
			}
			last = scanKey(prevNode, curLevel, node->key);
            mm->retire(hp,last);
			if (last != node || INVALID == node->next[curLevel].load(
					memory_order_relaxed)) {
				break;
			}
			/* the actual deletion of this node on the current level takes place */
			if (prevNode->next[curLevel].compare_swap(node, GET_UNMARKED(
					node->next[curLevel].load(memory_order_relaxed)))) {
				node->next[curLevel].store(INVALID, memory_order_relaxed);
				break;
			}
			if (INVALID == node->next[curLevel].load(memory_order_relaxed)) {
				break;
			}

			//TODO add backoff
		}

        mm->retire(hp,node);
		return prevNode;
	}

	   /*
        * REVIEW put this to util.h if it's general enough
        *
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

#endif /* LOCKFREE_DICTIONARY_H_ */
