/*
 * (c) Copyright 2008, IBM Corporation.
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef QUEUE_SHAVIT_H_
#define QUEUE_SHAVIT_H_
#include "smr.h"
#include <iostream>

namespace amino {
using namespace internal;

#define BACKOFF_TIME 10000

/**
 * @brief This is an implementation of a lock-free FIFO queue data structure. The
 * implementation is according to the paper An Optimistic Approach to Lock-Free
 * FIFO Queues by Edya Ladan-Mozes and Nir Shavit, 2004. To gain a complete
 * understanding of this data structure, please first read this paper, available
 * at: http://www.springerlink.com/content/h84dfexjftdal4p4/
 *<p>
 * First-in-first-out (FIFO) queues are among the most fundamental and highly
 * studied concurrent data structures. The most effective and practical
 * dynamic-memory concurrent queue implementation in the literature is the
 * lock-free FIFO queue algorithm of Michael and Scott, included in the standard
 * Java Concurrency Package. This paper presents a new dynamic-memory lock-free
 * FIFO queue algorithm that performs consistently better than the Michael and
 * Scott queue.
 *<p>
 * The key idea behind our new algorithm is a novel way of replacing the
 * singly-linked list of Michael and Scott, whose pointers are inserted using a
 * costly compare-and-swap (CAS) operation, by an optimistic doubly-linked list
 * whose pointers are updated using a simple store, yet can be fixed if a bad
 * ordering of events causes them to be inconsistent. It is a practical example
 * of an optimistic approach to reduction of synchronization overhead in
 * concurrent data structures.
 * <p>
 * LockFreeQueue maintain a doubly-linked list, but to construct the "backwards" direction, the path of prev pointers needed by dequeues.
 *<p>
 * Sample performance results here.
 *<p>
 * The following operations are thread-safe and scalable (but see notes in
 * method javadoc): first, offer, poll, peek
 *<p>
 * The following operations are not thread-safe: size, iterator
 *
 * @author Xiao Jun Dai
 *
 * @tparam T
 *            type of element in the queue
 */
template<typename T> class ShavitQueue {
private:
	/**
	 * Internal Node for queue.
	 */
	class QueueItem {
	public:
		/**
		 * data on the node.
		 */
		T data;
		/**
		 * Pointer to next node.
		 */
		QueueItem* next;
		/**
		 * Pointer to previous node.
		 * FIXME prev should not be atomic
		 */
		QueueItem* prev;

		QueueItem() :
			data() {
			next = NULL;
			prev = NULL;
		}

		QueueItem(const T& val) :
			data(val) {
			next = NULL;
			prev = NULL;
		}

		QueueItem(QueueItem* n) :
			data() {
			next = n;
			prev = NULL;
		}

		inline QueueItem* getNext() {
			return prev;
		}
	};

	/*
	 * Cannot be static since dummy->next is different in different queue objects.
	 * But one object need one dummy node only.
	 */
	QueueItem *dummy;

	/**
	 * Header pointer of queue.
	 */
	atomic<QueueItem *> head;
	/**
	 * Tail pointer of queue.
	 */
	atomic<QueueItem *> tail;
	/*
	 * SMR for memory management. All objects of the class queue share one SMR.
	 */
	SMR<QueueItem, 2>* mm;
    typedef typename SMR<QueueItem, 2>::HP_Rec HazardP;

	/**
	 * back off is for performance reason. if a CAS operation failed and the hasBackoff is true, the
	 * loop will delay wait_for_backoff millisecond for reducing conflict.
	 */
	static const bool hasBackoff = false;

	/* prevent the use of copy constructor and copy assignment*/
	ShavitQueue(const ShavitQueue& q) {
	}

	ShavitQueue& operator=(const ShavitQueue& q) {
	}

	void fixList(QueueItem *tail, QueueItem *head) {
		QueueItem *curNode, *curNodeNext;
		curNode = tail;
		while ((this->head.load(memory_order_relaxed) == head) && (curNode!= head)) {
			curNodeNext = curNode->next;
			if (NULL == curNodeNext) { /*Maybe ABA, return. I am not sure about this*/
				assert(false);
			}
			
            if (curNodeNext->prev != curNode) {
				curNodeNext->prev = curNode;
			}
			curNode = curNodeNext;
		}
	}
public:
	/**
	 * @brief constructor
	 */
	ShavitQueue() {
		mm = getSMR<QueueItem, 2> ();
		dummy = new QueueItem();
		head.store(dummy, memory_order_relaxed);
		tail.store(dummy, memory_order_relaxed);
	}

	/**
	 * @brief destructor
	 */
	virtual ~ShavitQueue() {
		QueueItem *first = head.load(memory_order_relaxed);
        QueueItem * ptail =  tail.load(memory_order_relaxed);
		
        while (ptail != dummy) {
			if (first == ptail) {
				delete first;
				break;
			}
			QueueItem *prev = first -> prev;
			delete first;
			first = prev;
		}

		delete dummy;
	}

	/**
	 * @brief Insert an element into the tail of queue
	 * @param x
	 * 			The element to be added
	 */
	void enqueue(const T& x) {
        HazardP * hp = mm->getHPRec();
#ifdef FREELIST
		QueueItem *node = mm->newNode(hp); /*Allocate a new node*/
        node->data =x;
#else
		QueueItem *node = new QueueItem(x);/*Allocate a new node*/
#endif

		int wait_for_backoff = BACKOFF_TIME;
		while (true) {
            QueueItem * t = tail.load(memory_order_relaxed); /*Get the tail*/

			/*hazard*/
            //FIXME hp as first parameter?
			mm->employ(hp,0, t);
			/*If the tail be modified, start over from the beginning of the loop*/
			if (tail.load(memory_order_relaxed) != t) {
				continue;
			}

			node->next = t;/*Set the node's next pointer*/
			if (tail.compare_swap(t, node)) {/*try to swing tail to the new node. If success, set the previous pointer*/
				t->prev = node;
				break;
			}

			if (hasBackoff) {
				usleep(wait_for_backoff);
				wait_for_backoff <<= 1;
			}
		}
	}

	/**
	 * @brief Remove a specified element by value
	 *
	 * @param val
	 * 			The element to be removed
	 * @return
	 * 			If the value exists in the queue, remove it and return true. else return false.
	 */
	bool dequeue(T& val) {
		QueueItem *tl, *hd, *fstNodePrev;

        HazardP * hp = mm->getHPRec();

		int wait_for_backoff = BACKOFF_TIME;
		while (true) {/*Try till success or empty*/
			hd = head.load(memory_order_relaxed);

			/*hazard*/
			mm->employ(hp, 0, hd);
			/*If the tail be modified, start over from the beginning of the loop*/
			if (head.load(memory_order_relaxed) != hd) {
				continue;
			}

			tl = tail.load(memory_order_relaxed);

			fstNodePrev = hd->prev;

			/*hazard*/
			mm->employ(hp, 1, fstNodePrev);
			//if (hd->prev != fstNodePrev) {
			//	continue;
			//}

			val = hd->data;

			if (head.load(memory_order_relaxed) == hd) {/*Check consistency*/
				/*
				 * Head value is dummy?
				 */
				if (hd != dummy) {
					if (tail.load(memory_order_relaxed) != head.load(
							memory_order_relaxed)) { /*More than 1 node?*/
						/**
						 *    If a prev pointer is found to be inconsistent, we run a fixList method along
						 *    the chain of next pointers which is guaranteed to be consistent
						 */
						if (NULL == fstNodePrev) {
							fixList(tl, hd);
							continue;
						}
					} else {/*Last node in queue*/
						dummy->next = tl;
						dummy->prev = NULL;
						if (tail.compare_swap(tl, dummy)) {/*CAS tail*/
							hd->prev = dummy;
						}
						continue;
					}
					/*CAS head*/
					if (head.compare_swap(hd, fstNodePrev)) {
						//FIXME free(head) free the dequeued node
						/* need SMR here */
						mm->retire(hp, 0);
                        mm->retire(hp, 1);
                        mm->delNode(hp, hd);
						return true;
					}
				} else {/*Head points to dummy*/
					if (tl == hd) {/*Tail points to dummy?*/
						//empty queue
						return false;
					} else {/*Need to skip dummy*/
						if (NULL == fstNodePrev) {/*prev pointer is inconsistency*/
							fixList(tl, hd);
							continue;
						}
						/*Skip dummy*/
						head.compare_swap(hd, fstNodePrev);
					}
				}
			}
			if (hasBackoff) {
				usleep(wait_for_backoff);
				wait_for_backoff <<= 1;
			}
		}
	}

	/**
	 * @brief Check to see if queue is empty
	 * @return true if empty, or false.
	 */
	bool empty() {
		QueueItem* tmp = head.load(memory_order_relaxed);
		return  tmp == dummy && tmp == tail.load(memory_order_relaxed);
	}

	/**
	 * @brief Get the size of the queue at this point
	 * @return the number of elements in the queue
	 */
	int size() {
		int ret = 0;
		QueueItem *first = head.load(memory_order_relaxed);
		
        while (tail.load(memory_order_relaxed) != dummy) {
			++ret;
			if (first == tail.load(memory_order_relaxed))
				break;
			first = first->prev;
		}
		return ret;
	}

	/**
	 * @brief Get the first element of the queue. If the queue is empty return false, else return
	 * true and assign the first to the parameter.
	 *
	 * @param ret
	 * 		The first element of the queue. It is valid if return true.
	 * @return
	 * 		If the queue is empty return false, else return true.
	 */
	bool peekFront(T& ret) {
		HazardP * hp = mm->getHPRec();

        while (true) {
			QueueItem *front = head.load(memory_order_relaxed);
			if (dummy != front) {
				ret = front->data;
				return true;
			}
            
			QueueItem *end = tail.load(memory_order_relaxed);
			if (front == end) {
				return false;
			} else {
                mm->employ(hp, 0, front); 
				while (head.load(memory_order_relaxed) != front)
                    continue;
                QueueItem *fstNodePrev = front->prev;
				if (NULL == fstNodePrev) {
					fixList(end, front);
					continue;
				}
				head.compare_swap(front, fstNodePrev); //yuzbj: is it necessary?
			}
			ret = front->data;
            mm->retire(hp, 0);
			return true;
		}
	}
};
}
#endif /*QUEUE_SHAVIT__H_*/
