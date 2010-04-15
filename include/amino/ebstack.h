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
#ifndef EBSTACK_H_
#define EBSTACK_H_

#include "smr.h"
#include "debug.h"

namespace amino {
/**
 * @brief The node type, which stores the data and a next pointer in it.
 *
 * @tparam T
 * 				Type of element stored in it
 */
template<typename T> class StackNode {
public:
	T data;
	StackNode* next;
	StackNode() :
		next(NULL) {
	}

	StackNode(const T& val) :
		data(val), next(NULL) {
	}
};
/**
 * <pre>
 * A Scalable Lock-free Stack Algorithm
 * Danny Hendler              Nir Shavit             Lena Yerushalmi
 * School of Computer Science Tel-Aviv University &  School of Computer Science
 * Tel-Aviv University        Sun Microsystems       Tel-Aviv University
 * Tel Aviv, Israel 69978     Laboratories           Tel Aviv, Israel 69978
 * hendlerd@post.tau.ac.il    shanir@sun.com         lenay@post.tau.ac.il
 * </pre>
 * @brief This a implementation of ebstack...
 * @tparam T
 * 				Type of element stored in it
 */
template<typename T> class EBStack {
private:
	atomic<StackNode<T>*> top; /*top pointer of stack*/
	SMR<StackNode<T> , 1>* mm; /*SMR for memory management*/

	/*collision array*/
	static const int TRY_TIMES = 4; /*times of try on collision array*/

    static StackNode<T> TOMB_STONE;
	static StackNode<T> REMOVED;

	const int coll_size; /*size of collision array*/
	atomic<StackNode<T>*>* coll_pop; /*collision array for pop*/
	atomic<StackNode<T>*>* coll_push; /*collision array for push*/
	atomic<int> position; /*position for try on collision array*/

	/*try to add element on collision array for pop*/
	bool try_add(StackNode<T>*);
	/*try to remove element on collision array for push*/
	StackNode<T>* try_remove();
	int get_rand_position();
	void initCollisionArray(int);
	void destoryCollisionArray();
    
    const int stime;
    
#ifdef DEBUG_STACK
    //record collision times.. yuezbj
	typedef struct _collTimes {
        _collTimes():pushCNum(0), pushCFNum(0),pushDNum(0), 
                     popCNum(0), popCFNum(0),popDNum(0)
        {
        }
        volatile int pushCNum ;
        volatile int pushCFNum;
        volatile int pushDNum ;
	    volatile int popCNum ;
        volatile int popCFNum ;
	    volatile int popDNum ;
    }CollTimes;
    /**This atomic CollTimes will record the collision times for all threads in one test case.
     */
    atomic<CollTimes*> pAllThreadCT;
    ThreadLocal<CollTimes*> *pLocalCTimes;

    FILE* fpushLog, *fpopLog;
#endif

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

public:
#ifdef DEBUG_STACK
    /**This function will be called in the test case before the thread exit.
     * So the test case only suitable for EBStack.
     */
    void freeCT()
    {
        CollTimes* ct =  pLocalCTimes->get();
        if (NULL != ct)
        {   
            CollTimes* ptmp;
            CollTimes* pnew = new CollTimes();

            while (!pAllThreadCT.compare_swap(ptmp, pnew))
            {
                ptmp = pAllThreadCT.load(memory_order_relaxed);

                pnew->pushCNum = (ct->pushCNum + ptmp->pushCNum);
                pnew->pushCFNum = (ct->pushCFNum + ptmp->pushCFNum);
                pnew->pushDNum = (ct->pushDNum + ptmp->pushDNum);
                pnew->popCNum = (ct->popCNum + ptmp->popCNum);
                pnew->popCFNum = (ct->popCFNum + ptmp->popCFNum);
                pnew->popDNum = (ct->popDNum + ptmp->popDNum);
            }

            //delete ptmp;
            delete ct;
        }
    }

    void logPush(char* log)
    {
        fwrite(log, sizeof(char), strlen(log), fpushLog); 
    }

    void logPop(char* log)
    {
        fwrite(log, sizeof(char), strlen(log), fpopLog);
    }
#endif 
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
	/**
	 * @brief Constructor with size of collision array.
	 *
	 * @param num size of collision array
	 */
	EBStack(int num = 8) :
		coll_size(num), position(), stime(300) {
        top.store(NULL, memory_order_relaxed);		
		mm = getSMR<StackNode<T> , 1> ();
#ifdef DEBUG_STACK
        fpushLog = fopen("ebstack_push.log", "a+");
        fpopLog = fopen("ebstack_pop.log", "a+");
        CollTimes* ptmp = new CollTimes();
        pAllThreadCT.store(ptmp, memory_order_relaxed);
        pLocalCTimes = new ThreadLocal<CollTimes*>(NULL); 
#endif
#ifdef DEBUG_CYCLE
        OpCycles* ptmp = new OpCycles();
        pAllThreadCycles.store(ptmp, memory_order_relaxed);
        pLocalCycles = new ThreadLocal<OpCycles*>(NULL);
#endif
		initCollisionArray(num);
	}

	/**
	 * @brief Destructor.
	 */
	~EBStack() {
#ifdef DEBUG_STACK
        fclose(fpushLog);
        fclose(fpopLog);
        CollTimes* ptmp = pAllThreadCT.load(memory_order_relaxed);
        cout<<"pushC:"<<ptmp->pushCNum<<" pushCF:"<<ptmp->pushCFNum<<" pushD:"<<ptmp->pushDNum<<endl;
        cout<<"popC:"<<ptmp->popCNum<<" popCF:"<<ptmp->popCFNum<<" popD:"<<ptmp->popDNum<<endl; 
        delete ptmp;
        delete pLocalCTimes;
#endif
#ifdef DEBUG_CYCLE
        OpCycles* ptmp = pAllThreadCycles.load(memory_order_relaxed);
        cout<<"pushCycles:"<<ptmp->pushC<<" popCycles:"<<ptmp->popC<<" peekTopCycles:"<<ptmp->peekTopC<<endl;
        delete ptmp;
        delete pLocalCycles;
#endif
        StackNode<T> *first = top.load(memory_order_relaxed);
		while (first != NULL) {
			StackNode<T> *next = first->next;
			delete first;
			first = next;
		}
		destoryCollisionArray();
	}

	void push(const T& d);
	bool pop(T& ret);
	bool empty();
	int size();
	bool peekTop(T& ret);

};

template<typename T>
StackNode<T> EBStack<T>::TOMB_STONE;
template<typename T>
StackNode<T> EBStack<T>::REMOVED;

/**
 * @brief Initialize the push and pop's collision array
 *
 * @param num
 * 			The size of the array
 */
template<typename T> void EBStack<T>::initCollisionArray(int num) {
	coll_push = new atomic<StackNode<T>*> [num];
	assert(NULL != coll_push);
	
	coll_pop = new atomic<StackNode<T>*> [num];
        assert(NULL != coll_pop);

	for (int i = 0; i < num; ++i) {
		coll_push[i].store(NULL);
		coll_pop[i].store(NULL);
	}
}

/**
 * @brief Delete the push and pop's collision array
 */
template<typename T> void EBStack<T>::destoryCollisionArray() {
	/*all elements in coll_push and coll_pop is unavailable now.
	 * no need to delete node put into coll_push and coll_pop first.
	 */
	delete[] coll_push;
	delete[] coll_pop;
}
/**
 * @brief Get a random position
 * @return A random position
 */
template<typename T> int EBStack<T>::get_rand_position() {
	return position++ % coll_size;
}

/**
 * @brief Push data onto Stack.
 *
 * @param d
 *            data to be pushed into the stack.
 */
template<typename T> void EBStack<T>::push(const T& d) {
#ifdef DEBUG_CYCLE
    OpCycles* oc = pLocalCycles->get();
    if ( NULL == oc)
    {
        oc = new OpCycles();
        pLocalCycles->set(oc);
    }

    unsigned long begin = cycleStamp();
#endif    

    StackNode<T>* newTop, *oldTop;
#ifdef FREELIST
    newTop = mm->newNode();
    newTop->data = d;
#else
	newTop = new StackNode<T> (d);
#endif

#ifdef DEBUG_STACK
    CollTimes* ct = pLocalCTimes->get();
    if ( NULL == ct)
    {
        ct = new CollTimes();
        pLocalCTimes->set(ct);
    } 
#endif 

	while (true) {
		/*no need to use a barrier*/
		oldTop = top.load(memory_order_relaxed);
		newTop->next = oldTop;

		if (top.compare_swap(oldTop, newTop)) {
#ifdef DEBUG_STACK
            ct->pushDNum++;
#endif
            break;
		}

        /*collision*/
        if (try_add(newTop)) {
#ifdef DEBUG_STACK
            ct->pushCNum++;
#endif
            break;
		}
#ifdef DEBUG_STACK
        else
        {
            ct->pushCFNum++;   
        }
#endif
    }

#ifdef DEBUG_CYCLE
    unsigned long end = cycleStamp();
    oc->pushC += (end-begin);
#endif
}

/**
 * @brief try_add...
 */
template<typename T> bool EBStack<T>::try_add(StackNode<T>* node) {
	int pos = get_rand_position();

	for (int i = 0; i < TRY_TIMES; ++i) {
		int index = (pos + i) % coll_size;

		StackNode<T>* pop_op = coll_pop[index].load(memory_order_relaxed);
		/*if some thread is waiting for removal, let's feed it with
		 * added object and return success.
		 */
		if (pop_op == &TOMB_STONE) {
			if (coll_pop[index].compare_swap(pop_op, node)) {
#ifdef DEBUG_STACK
                //char log[50];
                //sprintf(log, "%d:pushCNode_tomb: %x\n", (int)pthread_self(), node);
                //logPush(log);
#endif
                return true;
			}
		}
    }

	/*
	 * let's try to put adding objects to the buffer and
	 * waiting for removal threads.
	 */
    for (int i = 0; i < TRY_TIMES; ++i) {
		int index = (pos + i) % coll_size;
		StackNode<T>* push_op = coll_push[index].load(memory_order_relaxed);
		if (push_op == NULL) { /*this posiotion is empty*/
			if (coll_push[index].compare_swap(push_op, node)) {
				/* Now we check to see if added object is touched by removing threads.*/
			    usleep(stime);
                while (true) {
					push_op = coll_push[index].load(memory_order_relaxed);
					if (push_op == node) {
						if (coll_push[index].compare_swap(node, NULL)) {
							/*no removing thread touched us, return false*/
							return false;
						} else {
                            coll_push[index] = NULL;
#ifdef DEBUG_STACK
                            //char log[50];
                            //sprintf(log, "%d:pushCNode_node1: %x\n",(int)pthread_self(),node);
                            //logPush(log);
#endif
                            return true;
                        }
					} else {
						/*current value should be REMOVED. change REMOVED to be
						 * null means this position is available again.
						 */
						coll_push[index] = NULL;
#ifdef DEBUG_STACK
                        //char log[50];
                        //sprintf(log, "%d:pushCNode_node2: %x\n",(int)pthread_self(),node);
                        //logPush(log);
#endif
                        return true;
					}
				}
			}
		}
	}
	usleep(stime);
    return false;
}

/**
 * @brief Pop data from the Stack.
 * @param ret
 * 			topmost element of the stack. It is valid if the return value is true.
 *
 * @return If the value exists in the list, remove it and return true. else return false.
 */
template<typename T> bool EBStack<T>::pop(T& ret) {
#ifdef DEBUG_CYCLE
    OpCycles* oc = pLocalCycles->get();
    if ( NULL == oc)
    {
        oc = new OpCycles();
        pLocalCycles->set(oc);
    }

    unsigned long begin = cycleStamp();
#endif

	StackNode<T> *oldTop, *newTop = NULL;
    typename SMR<StackNode<T>, 1>::HP_Rec * hp = mm->getHPRec();

#ifdef DEBUG_STACK
    CollTimes* ct = pLocalCTimes->get();
    if ( NULL == ct)
    {
        ct = new CollTimes();
        pLocalCTimes->set(ct);
    } 
#endif 

    while (true) {
		oldTop = top.load(memory_order_relaxed);
		if (oldTop == NULL) {
			return false;
		}

		mm->employ(hp, 0, oldTop); //for memory management, hp0 = oldTop
		//memory barrier necessary here
		if (top.load(memory_order_relaxed) != oldTop) {
			continue;
		}

		newTop = oldTop->next;
		if (top.compare_swap(oldTop, newTop)) {
#ifdef DEBUG_STACK
            ct->popDNum++;
#endif
            break; //successful
		}

		// elimination backoff
		StackNode<T>* col_ele = try_remove();
		if (NULL != col_ele) {
			ret = col_ele->data;
#ifdef DEBUG_STACK		
            ct->popCNum++;
#endif	
            return true;
		}
#ifdef DEBUG_STACK	
        else
        {
            ct->popCFNum++;
        }
#endif	
	}
	mm->retire(hp,0);//hp0 = NULL. optional. for memory management

	ret = oldTop->data;
	mm->delNode(oldTop); //for memory management
#ifdef DEBUG_CYCLE
    unsigned long end = cycleStamp();
    oc->popC += (end-begin);
#endif

    return true;
}

/**
 * @brief try_remove...
 */
template<typename T> StackNode<T>* EBStack<T>::try_remove() {
    int pos = get_rand_position();
	
    for (int i = 0; i < TRY_TIMES; ++i) {
		int index = (pos + i) % coll_size;
		StackNode<T>* push_op = coll_push[index].load(memory_order_relaxed);
		// collide with pop
		if ((push_op != NULL) && (push_op != &REMOVED)) 
        {
            if (coll_push[index].compare_swap(push_op, &REMOVED)) {
#ifdef DEBUG_STACK
                //char log[50];
                //sprintf(log, "%d:popCNode_node: %x\n", (int)pthread_self(),push_op);
                //logPop(log);
#endif
                return push_op;
		    }
        }	
    }           
    
    for (int i = 0; i < TRY_TIMES; ++i) {
		int index = (pos + i) % coll_size;
            StackNode<T>* pop_op = coll_pop[index].load(memory_order_relaxed);
            if (pop_op == NULL) {
				if (coll_pop[index].compare_swap(pop_op, &TOMB_STONE)) {
                    usleep(stime);
					while (true) {
						pop_op = coll_pop[index].load(memory_order_relaxed);
						if (pop_op != &TOMB_STONE) {
							coll_pop[index].store(NULL, memory_order_relaxed);
#ifdef DEBUG_STACK
                            //char log[50];
                            //sprintf(log, "%d:popCNode_tomb: %x\n", (int)pthread_self(),pop_op);
                            //logPop(log);
#endif
                            return pop_op;
						} else {
							if (coll_pop[index].compare_swap(pop_op, NULL)) {
								return NULL;
							}
						}
					}
				}
			}
    }
    usleep(stime);
	return NULL;
}
/**
 * @brief Check to see if Stack is empty.
 *
 * @return true if stack is empty.
 */
template<typename T> bool EBStack<T>::empty() /*const*/{
	return top.load(memory_order_relaxed) == NULL;
}

/**
 * @brief Get the size of stack
 *
 * @return
 * 			The size of stack
 */
template<typename T> int EBStack<T>::size() {
	int ret = 0;
	StackNode<T> *node = top.load(memory_order_relaxed);
	while (node != NULL) {
		++ret;
		node = node->next;
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
template<typename T> bool EBStack<T>::peekTop(T& ret) {
#ifdef DEBUG_CYCLE
    OpCycles* oc = pLocalCycles->get();
    if ( NULL == oc)
    {
        oc = new OpCycles();
        pLocalCycles->set(oc);
    }

    unsigned long begin = cycleStamp();
#endif
    
    StackNode<T>* oldtop;
    typename SMR<StackNode<T>, 1>::HP_Rec * hp = mm->getHPRec();	
    while (true)
	{
		oldtop = top.load(memory_order_relaxed);
		if (oldtop == NULL)
			return false;
	    mm->employ(hp, 0, oldtop);

		if (top.load(memory_order_relaxed) != oldtop)
			continue;

		ret = oldtop->data;
		mm->retire(hp, 0);
#ifdef DEBUG_CYCLE
    unsigned long end = cycleStamp();
    oc->peekTopC += (end-begin);
#endif
    	return true;
	}	
}

}
#endif /*EBSTACK_H_*/
