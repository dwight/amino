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

#ifndef AMINO_QUEUE_H
#define AMINO_QUEUE_H

#include "smr.h"
#include <stdexcept>

namespace amino {

/*SMR is in internal namespace*/
using namespace internal;
/**
 * @brief This queue implementation is based on the algorithm defined in the
 * follwoing paper:
 *  Simple, fast, and practical non-blocking and blocking concurrent queue algorithms
 *  by Michael, M. M. and Scott, M. L. 1996. PODC96
 * ABA prevention: Maged M. Michael's SMR way
 * The following paper pointed out that Maged Michael's queue doesn't scale well.
 * Moir, M., Nussbaum, D., Shalev, O., and Shavit, N. 2005. Using elimination to
 * implement scalable and lock-free FIFO queues. In Proceedings of the Seventeenth
 * Annual ACM Symposium on Parallelism in Algorithms and Architectures
 * (Las Vegas, Nevada, USA, July 18 - 20, 2005). SPAA '05. ACM Press, New York, NY, 253-262.
 * <p>
 * However, from its performance test, SPAA05's scales much better than PODC96's
 * when number of CPU cores are large,  but performs slower when  few CPU cores
 * available. We decide to implement PODC96's for its simplicity and applicability
 * <p>
 * Performance tuning tips:
 * 1 backoff mechanism.
 * 2 Eliminate redundant memory barriers, implied by the atomics. Use atomic_xx_xx instead of
 *   neat C++ APIs, which are easy to use, but always imply full barriers currently.
 * TODO:
 * 1 code it in STL code style
 *
 * @author Mo Jiong Qiu
 * @author Xiao Jun Dai
 * @tparam T
 * 			The type of the element which is stored in the queue
 */

template<typename T> class LockFreeQueue {
private:
	/**
	 * @brief back off is for performance reason. if a CAS operation failed and the hasBackoff is true, the
	 * loop will delay wait_for_backoff millisecond for reducing conflict.
	 */
	static const bool hasBackoff = false;

	/*
	 * @brief Internal node used in the list-based queue
	 */
	class Node {
	public:
		T data;
		atomic<Node*> next;
		Node() {
			next.store(NULL, memory_order_relaxed);
		}

		Node(const T& val) :
			data(val) {
			next.store(NULL, memory_order_relaxed);
		}
	};

	/**
	 * Pointer to the head node. initially it point to a dummy node
	 */
	atomic<Node *> head;

	/**
	 * Pointer to the tail node. initially it point to a dummy node
	 */
	atomic<Node *> tail;

	/*for memory management*/
	SMR<Node, 2>* mm;

	/* prevent the use of copy constructor and copy assignment*/
	LockFreeQueue(const LockFreeQueue& q) {
	}

	LockFreeQueue& operator=(const LockFreeQueue& q) {
	}

public:
	/**
	 * @brief constructor
	 */
	LockFreeQueue() {
		/*get class's SMR*/
		mm = getSMR<Node, 2> ();

		/*dummy node. not get it from mm, for simplicity*/
		Node* dummy = new Node();
		head.store(dummy, memory_order_relaxed);
		tail.store(dummy, memory_order_relaxed);
	}

	/**
	 * @brief destructor
	 */
	~LockFreeQueue() {
		Node* first = head.load(memory_order_relaxed);
		Node* next = NULL;
        while (NULL != first) {
			next = (first->next).load(memory_order_relaxed);
			delete first;
			first = next;
		}
	}

	/**
	 * @brief insert an element into the tail of queue. Thread-safe
	 * @param d
	 * 			The element to be added
	 */
	void enqueue(const T& d) {
		Node *pTail ; /*tail*/
		Node *pTailNext ;

		int waitForBackoff = 1000;
#ifdef FREELIST
        Node * node = mm->newNode();
        node->data = d;
#else
		Node * node = new Node(d);
#endif
        typename SMR<Node, 2>::HP_Rec * hp = mm->getHPRec();

		while (true) {
			/* used as first parameter in compare_and_swap. compare_and_swap
			 * cannot accept NULL as its first parameter
			 */
			Node* temp = NULL;
			pTail = tail.load(memory_order_relaxed);
			/*hazard pointer, protected by SMR */
			mm->employ(hp, 0, pTail); 

			/*check if employ successful*/ 
			if (tail.load(memory_order_acquire) != pTail)
				continue;

			pTailNext = pTail->next.load(memory_order_relaxed);

			/*are latest tail and t_next consistent?*/
			if (tail.load(memory_order_relaxed) != pTail)
				continue;

			/*is tail pointing to the last node?*/
			if (pTailNext != NULL) {
				/*try to swing tail to the next node*/
				tail.compare_swap(pTail, pTailNext);
				continue;
			}

			/*enqueue by CAS tail pointer*/
			if (pTail->next.compare_swap(temp, node))
				break; /*enqueue is done*/

			if (hasBackoff) {
				usleep(waitForBackoff);
				waitForBackoff <<= 1;
			}
		}
		/*try to swing tail to the inserted node*/
		tail.compare_swap(pTail, node);

        //yuezbj: retire pTail
        mm->retire(hp, 0);
	}

	/**
	 * @brief remove and return an element from the head of queue. Thread-safe.
	 * @return return the head element of queue; return NULL(or 0) if queue is empty
	 */
	bool dequeue(T& ret) {
		Node *pHead ; 
		Node *pTail ;
		Node *pHeadNext ;
       
        typename SMR<Node, 2>::HP_Rec * hp = mm->getHPRec();

		int waitForBackoff = 1000;
		while (true) {
			pHead = head.load(memory_order_relaxed);
			/*hazard pointer, protected by SMR */
			mm->employ(hp, 1, pHead);
			/*check if employ successful*/ 
			if (head.load(memory_order_acquire) != pHead)
				continue;

			pHeadNext = pHead->next.load(memory_order_relaxed);
            mm->employ(hp, 1, pHeadNext); //pHead is local, needn't check

         	/*are head, tail, and h_next consistent?*/
			if (pHeadNext == NULL) /*empty queue*/
                return false;

            if (head.load(memory_order_relaxed) != pHead)
				continue;
			
			pTail = tail.load(memory_order_relaxed);
            if (pHead == pTail) { /*is tail falling behind*/
				tail.compare_swap(pTail, pHeadNext); /*try to advance pTail*/
				continue;
			}
			ret = pHeadNext->data;

			if (head.compare_swap(pHead, pHeadNext)) /*try to swing head to the next node*/
				break;

			if (hasBackoff) {
				usleep(waitForBackoff);
				waitForBackoff <<= 1;
			}
		}
        //yuezbj 
        mm->retire(hp, 1);//retire the node
		mm->delNode(pHead); /*free node from SMR*/
		return true;
	}

	/**
	 * @brief Check to see if queue is empty. Thread-safe.
	 * @return true if empty, or false.
	 */
	bool empty() {
		return NULL == head.load(memory_order_relaxed)->next.load(
				memory_order_relaxed);
	}

	/**
	 * @brief Get the size of the queue at this point. compute on the fly.
	 * Not thread-safe.
	 * @return the number of elements in the queue
	 */
	int size() {
		int result = 0;
		Node *front = head.load(memory_order_relaxed)->next.load(memory_order_relaxed);
		while (front != NULL) {
			++result;
			if (front == tail.load(memory_order_relaxed))
				break;
			front = front->next.load(memory_order_relaxed);
		}
		return result;
	}

	/**
	 * @brief Get the first element of the queue. If the queue is empty return false, else return
	 * true and assign the first to the parameter. Thread-safe.
	 *
	 * @param ret
	 * 		The first element of the queue. It is valid if return true.
	 * @return
	 * 		If the queue is empty return false, else return true.
	 */
	bool peekFront(T& top) {
		Node *front = NULL;
		typename SMR<Node, 2>::HP_Rec * hp = mm->getHPRec();

        while(true) {
		    front = (head.load(memory_order_relaxed)->next).load(memory_order_relaxed);
            if (front == NULL) {
			    return false;
		    }
            mm->employ(hp, 1, front);
            if (front != (head.load(memory_order_relaxed)->next).load(memory_order_relaxed))
                continue;
		    top = front->data;
            mm->retire(hp, 1);
		    return true;
        }
	}
};
}//namespace amino

#endif
