/*
 * (c) Copyright 2008, IBM Corporation.
 * Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#ifndef AMINO_STACK_H
#define AMINO_STACK_H

#include <amino/smr.h>
#include <amino/debug.h>
#include <stdexcept>

namespace amino {

using namespace internal;
/**
 * @brief This is a lock-free stack, based on:
 *  IBM, IBM System/370 Extended Architecture, Principles of Operation, 1983
 * ABA prevention method is based on Maged M. Michael's:
 * Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects
 *
 * @author Mo Jiong Qiu
 * @tparam T
 * 				Type of element stored in it
 */

/*
 * T must be a pointer or built-in type right now. 20070904 Mo Jiong Qiu
 * I will extend it according to STL potential rules, but for now ignore this extention
 *
 * backoff mechanism should be added to prevent high contention
 */
template<typename T> class LockFreeStack {
public:
	/**
	 * @brief The node type, which stores the data and a next pointer in it.
	 */
	class StackNode {
	public:
		/*the data*/
		T data;
		/*pointer to the next node*/
		StackNode* next;

		/**
		 * @brief Constructor. construct a node
		 */
		StackNode() {
			next = NULL;
		}

		/**
		 * @brief Constructor. construct a node
		 * @param val
		 * 			A specified element
		 */
		StackNode(const T& val) :
			data(val), next(NULL) {
		}
	};

#ifdef DEBUG_CYCLE
    void freeOC()
    {
        OpCycles* oc =  pLocalCycles->get();
        if (NULL != oc)
        {
            OpCycles* ptmp;
            OpCycles* pnew = new OpCycles();

            while (!pAllThreadCycles.compare_swap(ptmp, pnew))
            {
                ptmp = pAllThreadCycles.load(memory_order_relaxed);

                pnew->pushC = (oc->pushC + ptmp->pushC);
                pnew->popC = (oc->popC + ptmp->popC);
                pnew->peekTopC = (oc->peekTopC + ptmp->peekTopC);
            }

            //delete ptmp;
            delete oc;
        }
    }
#endif

private:
#ifdef DEBUG_CYCLE
    typedef struct _opCycles {
        _opCycles():pushC(0),popC(0),peekTopC(0)
        {
        }
        volatile unsigned long pushC;
        volatile unsigned long popC;
        volatile unsigned long peekTopC ;
    }OpCycles;

    atomic<OpCycles*> pAllThreadCycles;
    ThreadLocal<OpCycles*> *pLocalCycles;
#endif

#ifdef DEBUG_STACK
	atomic_int retries;
	atomic_int t_newNodes;
#endif

	atomic<StackNode *> top;
	SMR<StackNode, 1>* mm; //for memory management

	/* prevent the use of copy constructor and copy assignment*/
	LockFreeStack(const LockFreeStack& s) {
	}

	LockFreeStack& operator=(const LockFreeStack& st) {
	}

public:
	/**
	 *  @brief constructor
	 */
	LockFreeStack() {
#ifdef DEBUG_CYCLE
        OpCycles* ptmp = new OpCycles();
        pAllThreadCycles.store(ptmp, memory_order_relaxed);
        pLocalCycles = new ThreadLocal<OpCycles*>(NULL);
#endif
		mm = getSMR<StackNode, 1> (); //for memory management, here 1 hazard pointer is OK
		top.store(NULL, memory_order_relaxed);

#ifdef DEBUG_STACK
		retries = 0;
#endif
	}

#ifdef DEBUG_STACK
     //In order to pass compiling for perf_stack.h.
     void freeCT()
     {
     }
#endif

	/**
	 * @brief destructor
	 */
	~LockFreeStack() {
#ifdef DEBUG_CYCLE
        OpCycles* ptmp = pAllThreadCycles.load(memory_order_relaxed);
        cout<<"pushCycles:"<<ptmp->pushC<<" popCycles:"<<ptmp->popC<<" peekTopCycles:"<<ptmp->peekTopC<<endl;
        delete ptmp;
        delete pLocalCycles;
#endif
    	StackNode *first = top.load(memory_order_relaxed);
		while (first != NULL) {
			StackNode *next = first->next;
			delete first;
			first = next;
		}
	}

	/**
	 * @brief Pop data from the Stack.
	 * @param ret
	 * 			topmost element of the stack. It is valid if the return value is true.
	 *
	 * @return If the value exists in the list, remove it and return true. else return false.
	 */
	bool pop(T& ret) {
#ifdef DEBUG_CYCLE
    OpCycles* oc = pLocalCycles->get();
    if ( NULL == oc)
    {
        oc = new OpCycles();
        pLocalCycles->set(oc);
    }

    unsigned long begin = cycleStamp();
#endif
	
    	StackNode *oldTop, *newTop;
        typename SMR<StackNode, 1>::HP_Rec * hp = mm->getHPRec();

		while (true) {
			oldTop = top.load(memory_order_relaxed);//no need to imply membar
			if (oldTop == NULL)
				return false;

			mm->employ(hp, 0, oldTop); //for memory management, hp0 = oldTop

			if (top.load(memory_order_relaxed) != oldTop)
				continue;
			newTop = oldTop->next;
			if (top.compare_swap(oldTop, newTop))
				break; //yes, successful
#ifdef DEBUG_STACK
			retries++;
#endif
		}
		mm->retire(hp, 0);//hp0 = NULL. optional. for memory management

		ret = oldTop->data;
		mm->delNode(hp, oldTop); //for memory management
#ifdef DEBUG_CYCLE
    unsigned long end = cycleStamp();
    oc->popC += (end-begin);
#endif
		return true;
	}

	/**
	 * @brief Push data onto Stack.
	 *
	 * @param d
	 *            data to be pushed into the stack.
	 */
	void push(const T& d) {
#ifdef DEBUG_CYCLE
    OpCycles* oc = pLocalCycles->get();
    if ( NULL == oc)
    {
        oc = new OpCycles();
        pLocalCycles->set(oc);
    }

    unsigned long begin = cycleStamp();
#endif
		
        StackNode*oldTop, *newTop;

#ifdef DEBUG_STACK
		struct timeval start, end;
		gettimeofday(&start, NULL);
#endif

#ifdef FREELIST
                newTop = mm->newNode();
#else
		newTop = new StackNode();
#endif

#ifdef DEBUG_STACK
		gettimeofday(&end, NULL);
		t_newNodes = 1000000*(end.tv_sec-start.tv_sec) + (end.tv_usec-start.tv_usec);
#endif

		newTop->data = d;

		/* do we check whether newTop is NULL here? */
		do {
			oldTop = top.load(memory_order_relaxed); //no need to use a barrier
			newTop->next = oldTop;
		} while (top.compare_swap(oldTop, newTop) == false);
#ifdef DEBUG_CYCLE
    unsigned long end = cycleStamp();
    oc->pushC += (end-begin);
#endif
	}

	/**
	 * @brief Check to see if Stack is empty.
	 *
	 * @return true if stack is empty.
	 */
	bool empty() /*const*/{
		return top.load(memory_order_relaxed) == NULL;
	}

	/**
	 * @brief Get the size of stack
	 *
	 * @return
	 * 			The size of stack
	 */
	int size() {
		int ret = 0;
		StackNode *node = top.load(memory_order_relaxed);
		while (node != NULL) {
			++ret;
			node = node -> next;
		}
		return ret;
	}

	/**
	 * @brief Get the topmost element in the stack. If the stack is empty, it will return false.
	 * else return true and assign the topmost element to the parameter.
	 *
	 * @param ret
	 * 			The topmost element in the stack. It is valid if the return value is true.
	 * @return
	 * 			If the stack is empty return false, else return true.
	 */
	bool peekTop(T& ret) {
        // FIXME: We need to fix here by using SMR to protect top node
	// Fixed by yuezbj 2009/03/11
#ifdef DEBUG_CYCLE
    OpCycles* oc = pLocalCycles->get();
    if ( NULL == oc)
    {
        oc = new OpCycles();
        pLocalCycles->set(oc);
    }

    unsigned long begin = cycleStamp();
#endif
      
        StackNode*oldTop;
            typename SMR<StackNode, 1>::HP_Rec * hp = mm->getHPRec();
	    
	    while(true) {
		oldTop = top.load(memory_order_relaxed);
		if (oldTop == NULL)
			return false;

		mm->employ(hp, 0, oldTop); 
		if (top.load(memory_order_relaxed) != oldTop)
			continue;
		ret = oldTop->data;
		mm->retire(hp, 0);
#ifdef DEBUG_CYCLE
    unsigned long end = cycleStamp();
    oc->peekTopC += (end-begin);
#endif
		return true;
	    }
	}

#ifdef DEBUG_STACK
	int getCASRetries() {
		return retries.load(memory_order_relaxed);
	}

	int getTimeNewNodes() {
		return t_newNodes.load(memory_order_relaxed);
	}
#endif

};

}
#endif

