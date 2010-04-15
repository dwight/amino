/*
 * (c) Copyright 2008, IBM Corporation.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
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
 *
 *  Change History:
 *
 *  yy-mm-dd  Developer  Defect     Description
 *  --------  ---------  ------     -----------
 *  08-08-03  ganzhi     N/A        Initial implementation
 *  09-04-03  Nick       N/A        PPC version
 */

#ifndef AMINO_DEQUE_H
#define AMINO_DEQUE_H

#include <stdexcept>

#include <amino/util.h>
#include <amino/smr.h>

#ifndef CASD
#include <amino/mutex.h>
#endif

#ifdef DEBUG
#include <amino/debug.h>
#endif

namespace amino {
	/* needed by SMR */
	using namespace internal;

	/*
	 * A deque is stable only if the status tag holds the value stable.
	 * A deque can be stable only if it is coherent. That is, for all
	 * nodes x in the deque's list x.Right.Left=x, unless x is the
	 * rightmost node; and x.Left.Right=x, unless x is the leftmost node.
	 *  Empty and single-item deques are always stable.A deque can be
	 *  unstable only if it contains two or more items. It is acceptable
	 *  for one end of the deque to be incoherent, as long as the status
	 *  tag indicates an unstable deque on the same end. The list,
	 *  excluding the end nodes, is always coherent. The deque is
	 *  right-incoherent if r.Left.Right=r, where r is the rightmost node.
	 *  The deque is left-incoherent if l.Right.Left=l, where l is the
	 *  leftmost node. And the status tag is rpush and lpush respectively.
	 *  At most one end of the deque can be incoherent at a time.
	 *  Unstable states result from push operations, but never from pop
	 *  operations, hence the aming of the unstable status tag values.
	 *  It is also acceptable for the status tag to indicate an unstable
	 *  deque even when the deque is in fact coherent.
	 */
#define STABLE 0
#define RPUSH 1
#define LPUSH 2

	/**
	 * This Deque implementation is based on the algorithm defined in the following
	 * paper: CAS-Based Lock-Free Algorithm for Shared Deques By Maged M. Michael
	 *
	 * The deque class
	 *
	 * @param T type of elements in the deque
	 */
    template<typename T> 
    class Node {
    public:
	    /**
    	 * element on the node.
	     */
    	T data;

	    /**
    	 * point to left node.
	     */
    	atomic<Node*> left;

	    /**
    	 * point to right node.
	     */
    	atomic<Node*> right;

	    Node() : data() {
		    left.store(NULL, memory_order_relaxed);
    		right.store(NULL, memory_order_relaxed);
	    }

    	Node(const T& val) :
	    data(val) {
		    left.store(NULL, memory_order_relaxed);
    		right.store(NULL, memory_order_relaxed);
	    }
    };

    typedef struct All{
        volatile unsigned long low;
        volatile unsigned long high;
    }ALL;

    /**
    * @brief This anonymous union object is used to store data of Anchor.
    */
    template<typename T>
    union AnchorValue{
        /**
          * @brief enable coarse grained access to internal states
          */
        volatile struct All all;
        AnchorValue& operator= (const AnchorValue& av)
        {
            if (this == &av)
                return *this;

            all.low = av.all.low;
            all.high = av.all.high;
            return *this;
        }
        /**
          * @brief enable fine grained access to internal states
          */
        volatile struct {
            /* pointer to left nodes, 32 bits */
            volatile unsigned long left;

            /* pointer to right nodes. 30 bits.
             * The LSB(two bits) is used by <code>status</code> field.
             */
            volatile unsigned long right :(sizeof(unsigned long) * 8 - 2);

            /* status of the Anchor. 2 bits.
             * it can be one of the "STABLE", "LPUSH", "RPUSH"
             */
             volatile unsigned long status :2;//status type
        }fields;

        void setRight(Node<T> * pNode) {
            fields.right = ((unsigned long) pNode) >> 2;
            assert(getRight() == pNode);
        }

        void setLeft(Node<T>* pNode)
        {
            fields.left = (unsigned long)pNode;
        }

        void setStatus (unsigned long status)
        {
            fields.status = status;
        }

        Node<T> * getRight()  
        {
            return (Node<T> *) (fields.right << 2);
        }

        Node<T> * getLeft() 
        {
            return (Node<T> *) fields.left;
        }

        unsigned long getStatus() 
        {
            return fields.status;
        }
    };

    /**
     * @brief Definition of anchor which is used to store head and tail pointer,
     * and status of the deque.
     *
     * This class must be allocated in an aligned address.
     */
    template<typename T>
    class  AnchorType {
    public:
#ifdef PPC
        static SMR<AnchorValue<T>, 1>* mm_av; 
        typedef typename SMR<AnchorValue<T>, 1>::HP_Rec HazardP_AV;
        atomic<AnchorValue<T>*> pvalue;

        AnchorType()
        {
            pvalue.store(NULL, memory_order_relaxed);
        }

        /*
         * This function will allocate the memory aligned anchor value.
         */
        static AnchorValue<T>* newAnchorValue() 
        {
            AnchorValue<T>* myvalue;
#ifdef FREELIST
            myvalue = mm_av->newNode();
            if (NULL != myvalue)
            {
                return myvalue ; 
            }
#endif                
            int ret = 0;
#if defined(BIT32)
            ret = posix_memalign((void**)&myvalue, 8, sizeof(ALL));
#elif defined(BIT64)
            ret = posix_memalign((void**)&myvalue, 16, sizeof(ALL));
#endif
            assert(ret == 0);
            assert(NULL != myvalue);
            assert((unsigned long)myvalue % 4 == 0);
            return myvalue ;               
        }

#else

#if defined(BIT32)
        AnchorValue<T> value __attribute__((aligned(8)));
#elif defined(BIT64)
        AnchorValue<T> value __attribute__((aligned(16)));
#endif
#endif
        /*
         * This function will free the memory pointed by pvalue when destructing the deque.
         * It can't be the destructor of AnchorType because the memory should be managed by SMR and 
         * it can't be freed each time the object of AnchorType destructs.
         */
        void destroy()
        {
#ifdef PPC
            AnchorValue<T>* tmp = pvalue.load(memory_order_relaxed);
            if (NULL != tmp);
                free(tmp);

            delete mm_av;
#endif
        }

        void init()
        {
#ifdef PPC
            AnchorValue<T>* myValue = newAnchorValue();
            assert(NULL != myValue);
            myValue->all.high = 0;
            myValue->all.low = 0;
            pvalue.store(myValue, memory_order_relaxed);
#else
            value.all.high = 0;
            value.all.low = 0;
#endif                
        }

        /**
         * This function will pack the <code>pNode</code> pointer into right field.
         * Since we used LSB of right field to store status, the lowest
         * two bits of pNode must be zero.
         */
        void setRight(Node<T> * pNode) {
#ifdef PPC
            AnchorValue<T>* value = pvalue.load(memory_order_relaxed); 
            if (NULL == value)
            {
                AnchorValue<T>* tmp = newAnchorValue();
                tmp->setRight(pNode);
                pvalue.store(tmp, memory_order_relaxed);
            } else {
                value->fields.right = ((unsigned long) pNode) >> 2;                    
            }
#else            
            value.fields.right = ((unsigned long) pNode) >> 2;
            assert(getRight() == pNode);
#endif
        }

        void setLeft(Node<T>* pNode)
        {
#ifdef PPC
            AnchorValue<T>* value = pvalue.load(memory_order_relaxed); 
            if (NULL == value)
            {
                AnchorValue<T>* tmp = newAnchorValue();
                tmp->fields.left = (unsigned long)pNode;
                pvalue.store(tmp, memory_order_relaxed);
            } else {
                value->fields.left = (unsigned long)pNode;                    
            }
#else                       
            value.fields.left = (unsigned long)pNode;
#endif                
        }

        void setStatus(unsigned long status)
        {
#ifdef PPC
            AnchorValue<T>* value = pvalue.load(memory_order_relaxed); 
            if (NULL == value)
            {
                AnchorValue<T>* tmp = newAnchorValue();
                tmp->setStatus(status);
                pvalue.store(tmp, memory_order_relaxed);
            } else {
                value->fields.status = status;                    
            }
#else                       
            value.fields.status = status;
#endif             
        }
        Node<T> * getRight() 
        {
#ifdef PPC    
            return (pvalue.load(memory_order_relaxed))->getRight();
#else
            return (Node<T> *) (value.fields.right << 2);
#endif                
        }

        Node<T> * getLeft() 
        {
#ifdef PPC    
            return (pvalue.load(memory_order_relaxed))->getLeft();
#else
            return (Node<T> *) value.fields.left;
#endif                
        }

        unsigned long getStatus() 
        {
#ifdef PPC    
            return (pvalue.load(memory_order_relaxed))->getStatus();
#else
            return value.fields.status;
#endif             
        }
        /**
         * This function tries to solve one interesting problem:
         * load of two long integer is not atomic.
         *
         * @param newAnchor
         *      we will try to copy the anchor to the newAnchor in an atomic way.
         */
        void copyTo(AnchorType<T>& newAnchor) __attribute__((noinline)) {
#ifdef PPC
            HazardP_AV* hp = mm_av->getHPRec();
            while (true){
                AnchorValue<T>* tmp = pvalue.load(memory_order_relaxed);
                mm_av->employ(hp, 0, tmp);
                if (tmp != pvalue.load(memory_order_relaxed))
                    continue;

                newAnchor.pvalue.store(tmp, memory_order_relaxed);
                break;
            }  
#else
            do {
                newAnchor.value.all.low = value.all.low;
                newAnchor.value.all.high = value.all.high;
            }while (newAnchor.value.all.low != value.all.low || newAnchor.value.all.high != value.all.high);
#endif 
        }

        bool operator == (const AnchorType<T>& atr)
        {
#ifdef PPC
            if (pvalue.load(memory_order_relaxed) != atr.pvalue.load(memory_order_relaxed))
                return false;
#else
            if (value.all.high != atr.value.all.high ||
                    value.all.low != atr.value.all.low) {
                return false;
            }
#endif
            return true;   
        }

        bool compare_swap(AnchorType<T>* expected, AnchorType<T>* newValue) volatile{
#ifdef CASD
            return casd(&(value.all), (void *)&(expected->value.all), (void *)&(newValue->value.all));
#endif
#ifdef PPC  
            AnchorValue<T>* exp = expected->pvalue.load(memory_order_relaxed);
 
            if(pvalue.compare_swap(exp, newValue->pvalue.load(memory_order_relaxed)))
            {
                return true;
            }
        
            return false;
#endif                
        }

    };

#ifdef PPC
    /*
     * For memory management by SMR. The class AnchorType has only one SMR.
     * The input param 1 means this SMR will manage the aligned memory.
     */
    template <typename T>
    SMR<AnchorValue<T>, 1>* AnchorType<T>::mm_av = getSMR<AnchorValue<T>, 1> (1);
#endif        

    template<typename T>
    class LockFreeDeque {
    private:
        /**
         * Now we forbid the use of copy constructor and copy assignment.
         * It's very difficult to create a lock-free copy constructor and
         * copy assignment.
         */
        LockFreeDeque(const LockFreeDeque& dq) {
        }

        LockFreeDeque& operator=(const LockFreeDeque& dq) {
        }

        SMR<Node<T>, 3>* mm; /* SMR for memory management */
        typedef typename SMR<Node<T>, 3>::HP_Rec HazardP;

        AnchorType<T> anchor;
#ifdef DEBUG_DEQUE
        //record operation times.. yuezbj
        typedef struct _opTimes {
            _opTimes():pushRNum(0), pushLNum(0),popRNum(0), popLNum(0)
            {
            }
            volatile int pushRNum ;
            volatile int pushLNum ;
            volatile int popRNum ;
            volatile int popLNum ;
        }OPTimes;
        /**This atomic OPTimes will record the collision times for all threads in one test case.
        */
        atomic<OPTimes*> pAllThreadCT;
        ThreadLocal<OPTimes*> *pLocalCTimes;

        FILE* fpushLog, *fpopLog;
#endif

    public:
#ifdef DEBUG_DEQUE
    /**This function will be called in the test case before the thread exit.
     */
    void freeCT()
    {
        OPTimes* ct =  pLocalCTimes->get();
        if (NULL != ct)
        {   
            OPTimes* ptmp;
            OPTimes* pnew = new OPTimes();

            while (!pAllThreadCT.compare_swap(ptmp, pnew))
            {
                ptmp = pAllThreadCT.load(memory_order_relaxed);

                pnew->pushRNum = (ct->pushRNum + ptmp->pushRNum);
                pnew->pushLNum = (ct->pushLNum + ptmp->pushLNum);
                pnew->popRNum = (ct->popRNum + ptmp->popRNum);
                pnew->popLNum = (ct->popLNum + ptmp->popLNum);
            }

            //delete ptmp;
            delete ct;
        }
    }

    void logPush(char* log)
    {
        //fwrite(log, sizeof(char), strlen(log), fpushLog); 
    }

    void logPop(char* log)
    {
        //fwrite(log, sizeof(char), strlen(log), fpopLog);
    }
#endif 
        /**
         * This constructor will create an empty deque.
         **/
        LockFreeDeque() {
            /* Get an SMR for memory management*/
            m = getSMR<Node<T>, 3> ();
            /* set initial status of anchor: "pointers null, status STABLE" */
            anchor.init();
#ifdef DEBUG_DEQUE
            fpushLog = fopen("deque_push.log", "a+");
            fpopLog = fopen("deque_pop.log", "a+");
            OPTimes* ptmp = new OPTimes();
            pAllThreadCT.store(ptmp, memory_order_relaxed);
            pLocalCTimes = new ThreadLocal<OPTimes*>(NULL); 
#endif
        }

        /**
         * put element d into the right of deque.
         *
         * @param d element to be put
         */
        void enqueue(const T &d) {
            pushRight(d);
        }

        /**
         * get a element from the left of deque.
         *
         * @return elment
         */
        bool dequeue(T& ret) {
            return popLeft(ret);
        }

        /**
         *  destructor.
         */
        virtual ~LockFreeDeque() {
#ifdef DEBUG_DEQUE
            fclose(fpushLog);
            fclose(fpopLog);
            OPTimes* ptmp = pAllThreadCT.load(memory_order_relaxed);
            cout<<"pushRNum:"<<ptmp->pushRNum<<" pushLNum:"<<ptmp->pushLNum<<endl;
            cout<<"popRNum:"<<ptmp->popRNum<<" popLNum:"<<ptmp->popLNum<<endl; 
            delete ptmp;
            delete pLocalCTimes;
#endif
            Node<T>* left = anchor.getLeft();
            Node<T>* right = anchor.getRight();
            if(left!=NULL) {
                Node<T> * current = left;
                while(current!=right) {
                    Node<T> * next = current->right.load(memory_order_relaxed);
                    delete current;
                    current = next;
                }
                delete right;
            }
            anchor.destroy();
        }

        /**
         * This method pushes data into right end of deque.
         *
         * @param data
         *      element to be added
         */
        virtual void pushRight(const T& data) __attribute__((noinline)) {
            AnchorType<T> old_anchor, new_anchor;

#ifdef FREELIST
            Node<T>* node = mm->newNode();
            assert(NULL != node);
            node->data = data;
#else
            /*create a new node for storing data*/
            Node<T>* node = new Node<T>(data);
#endif
            /* assertion for judging the last two bits is zero
               and is free for reusing */
            assert(((unsigned long) node) % 4 == 0);
#ifdef DEBUG_DEQUE
            OPTimes* ct = pLocalCTimes->get();
            if ( NULL == ct)
            {
                ct = new OPTimes();
                pLocalCTimes->set(ct);
            } 
#endif
#ifdef PPC
            typename SMR<AnchorValue<T>, 1>::HP_Rec* hp = AnchorType<T>::mm_av->getHPRec();
#endif 
            /* loop until succeed */
            while (true) {
                /*reading a fresh snapshot of Anchor to local */
                anchor.copyTo(old_anchor);
                Node<T>* left = old_anchor.getLeft();
                Node<T>* right = old_anchor.getRight();
                unsigned long status = old_anchor.getStatus();
                /*deque is empty*/
                if (right == 0) {
                    assert(left == 0);
                    /*set up a new anchor <node, node,s>*/
                    new_anchor.setLeft(node);
                    new_anchor.setRight(node);
                    new_anchor.setStatus(STABLE);
#ifdef PPC
                    AnchorValue<T>* oldan = old_anchor.pvalue.load(memory_order_relaxed);
                    AnchorType<T>::mm_av->employ(hp, 0, oldan);
                    if (oldan != old_anchor.pvalue.load(memory_order_relaxed))
                        continue;
#endif
                    /*
                     * Insert data into to empty deque. There is no need to
                     * stablize deque. CAS fails when others CAS changed the state
                     * of deque, the process restarts the push attempt by reading a
                     * fresh snapshot of Anchor
                     */
                    if (anchor.compare_swap(&old_anchor, &new_anchor))  {
#ifdef DEBUG_DEQUE
                        ct->pushRNum++;
                        char log[50];
                        sprintf(log, "%d:pushRNode: %x\n", (int)pthread_self(), (unsigned int)node);
                        logPush(log);
#endif
#ifdef PPC
                        AnchorType<T>::mm_av->retire(hp, 0);
                        AnchorType<T>::mm_av->delNode(hp, oldan);
#endif
                        return;
                    }

                }
                /*deque is stable*/
                else if (status == STABLE) {
                    assert(right != NULL);
                    node->left.store(right, memory_order_acquire);
                    assert(node->left.load() == right );

                    /*set up a new anchor <l, node, RPUSH>*/
                    new_anchor.setLeft(left);
                    new_anchor.setRight(node);
                    new_anchor.setStatus(RPUSH);
#ifdef PPC 
                    AnchorValue<T>* oldan = old_anchor.pvalue.load(memory_order_relaxed);
                    AnchorType<T>::mm_av->employ(hp, 0, oldan);
                    if (oldan != old_anchor.pvalue.load(memory_order_relaxed))
                        continue;
#endif
                    /*
                     * Replace the anchor with new pointers, if it
                     * succeeds, the anchor is not stable now.
                     * CAS fails when others CAS changed the state
                     * of deque, the process restarts the push attempt by reading a
                     * fresh snapshot of Anchor
                     */
                    if (anchor.compare_swap(&old_anchor, &new_anchor)) {
                        stabilizeRight(new_anchor);
#ifdef PPC
                        AnchorType<T>::mm_av->retire(hp, 0);
                        AnchorType<T>::mm_av->delNode(hp, oldan);
#endif
#ifdef DEBUG_DEQUE
                        char log[50];
                        sprintf(log, "%d:pushRNode: %x\n", (int)pthread_self(), (unsigned int)node);
                        logPush(log);
                        ct->pushRNum++;
#endif
                        return;
                    }
                }
                /* deque is unstable. stabilize it first. */
                else {
                    stabilize(old_anchor);
                }
            }
        }

        /**
         * This method pushes data into left end of deque.
         *
         * @param data
         *      element to be added
         */
        virtual void pushLeft(const T& data) __attribute__((noinline)) {
            AnchorType<T> old_anchor, new_anchor;

#ifdef FREELIST
            Node<T>* node = mm->newNode();
            assert(NULL != node);
            node->data = data;
#else
            /*create a new node for storing data*/
            Node<T>* node = new Node<T>(data);
            assert(NULL != node);
#endif
            /* assertion for judging the last two bits is zero
               and is free for reusing */
            assert(((unsigned long) node) % 4 == 0);
#ifdef DEBUG_DEQUE
            OPTimes* ct = pLocalCTimes->get();
            if ( NULL == ct)
            {
                ct = new OPTimes();
                pLocalCTimes->set(ct);
            } 
#endif
#ifdef PPC
            typename SMR<AnchorValue<T>, 1>::HP_Rec* hp = AnchorType<T>::mm_av->getHPRec();
#endif
            // loop until succeed
            while (true) {
                /*reading a fresh snapshot of Anchor to local */
                anchor.copyTo(old_anchor);
                Node<T>* left = old_anchor.getLeft();
                Node<T>* right = old_anchor.getRight();
                unsigned long status = old_anchor.getStatus();
                /*deque is empty*/
                if (left == 0) {
                    assert(right == 0);
                    /*set up a new anchor <node, node,s>*/
                    new_anchor.setLeft(node);
                    new_anchor.setRight(node);
                    new_anchor.setStatus(STABLE);
#ifdef PPC
                    AnchorValue<T>* oldan = old_anchor.pvalue.load(memory_order_relaxed);
                    AnchorType<T>::mm_av->employ(hp, 0, oldan);
                    if (oldan != old_anchor.pvalue.load(memory_order_relaxed))
                        continue;
#endif
         
                   /*
                     * Insert data into to empty deque. There is no need to
                     * stablize deque. CAS fails when others CAS changed the state
                     * of deque, the process restarts the push attempt by reading a
                     * fresh snapshot of Anchor
                     */
                    if (anchor.compare_swap(&old_anchor, &new_anchor)) {
#ifdef DEBUG_DEQUE
                        char log[50];
                        sprintf(log, "%d:pushLNode: %x\n", (int)pthread_self(), (unsigned int)node);
                        logPush(log);
                        ct->pushLNum++;
#endif
#ifdef PPC
                        AnchorType<T>::mm_av->retire(hp, 0);
                        AnchorType<T>::mm_av->delNode(hp, oldan);
#endif
                        return;
                    }
                }
                /*deque is stable*/
                else if (status == STABLE) {
                    assert(left != NULL);
                    node->right.store(left, memory_order_acquire);
                    assert(node->right.load() == left);
                    
                    /*set up a new anchor <r, node, LPUSH>*/
                    new_anchor.setRight(right);
                    new_anchor.setLeft(node);
                    new_anchor.setStatus(LPUSH);
#ifdef PPC
                    AnchorValue<T>* oldan = old_anchor.pvalue.load(memory_order_relaxed);
                    AnchorType<T>::mm_av->employ(hp, 0, oldan);
                    if (oldan != old_anchor.pvalue.load(memory_order_relaxed))
                        continue;
#endif
                    /*
                     * Replace the anchor with new pointers, if it
                     * succeeds, the anchor is not stable now.
                     * CAS fails when others CAS changed the state
                     * of deque, the process restarts the push attempt by reading a
                     * fresh snapshot of Anchor
                     */
                    if (anchor.compare_swap(&old_anchor, &new_anchor)) {
                        stabilizeLeft(new_anchor);
#ifdef PPC
                        AnchorType<T>::mm_av->retire(hp, 0);
                        AnchorType<T>::mm_av->delNode(hp, oldan);
#endif

#ifdef DEBUG_DEQUE
                        char log[50];
                        sprintf(log, "%d:pushLNode: %x\n", (int)pthread_self(), (unsigned int)node);
                        logPush(log);
                        ct->pushLNum++;
#endif
                        return ;
                    }
                }
                /* deque is unstable. stabilize it first. */
                else {
                    stabilize(old_anchor);
                }
            }
        }

        /**
         * @brief Remove one element from the right end of deque. If the deque is empty, return false. esle
         * return true and assign the most right element to the parameter.
         *
         * @param ret
         * 		The right most element of the deque. It is valid if the return value is true.
         * @return
         *      If the deque is not empty, return true. else return false.
         */
        virtual bool popRight(T& ret) __attribute__((noinline)) {
            AnchorType<T> old_anchor, new_anchor;
            Node<T>* prev;

            HazardP * hp = mm->getHPRec();
#ifdef PPC
            typename SMR<AnchorValue<T>, 1>::HP_Rec* hp_av = AnchorType<T>::mm_av->getHPRec();
#endif
#ifdef DEBUG_DEQUE
            OPTimes* ct = pLocalCTimes->get();
            if ( NULL == ct)
            {
                ct = new OPTimes();
                pLocalCTimes->set(ct);
            } 
#endif 

            while (true) {
                anchor.copyTo(old_anchor);
                Node<T>* left = old_anchor.getLeft();
                Node<T>* right = old_anchor.getRight();
                unsigned long status = old_anchor.getStatus(); 
                /*deque is empty*/
                if (right == 0) {
                    assert(left == 0);
                    assert(status == STABLE);
                    return false;
                }
                /*only one element in deque*/
                if (left == right) {
                    /*create a new anchor <null,null,s>*/
                    mm->employ(hp, 1, right);
                    if (right != anchor.getRight())
                        continue;
                    new_anchor.setStatus(status);
                    new_anchor.setLeft(0);
                    new_anchor.setRight(0);
#ifdef PPC
                    AnchorValue<T>* oldan = old_anchor.pvalue.load(memory_order_relaxed);
                    AnchorType<T>::mm_av->employ(hp_av, 0, oldan);
                    if (oldan != old_anchor.pvalue.load(memory_order_relaxed))
                        continue;
#endif
                    if (anchor.compare_swap(&old_anchor, &new_anchor)) {
                        ret = right->data;
                        mm->retire(hp, 1);
                        mm->delNode(hp, right);
#ifdef DEBUG_DEQUE
                        char log[50];
                        sprintf(log, "%d:popRNode: %x\n", (int)pthread_self(), (unsigned int)right);
                        logPop(log);
                        ct->popRNum++;
#endif
#ifdef PPC
                        AnchorType<T>::mm_av->retire(hp_av, 0);
                        AnchorType<T>::mm_av->delNode(hp_av, oldan);
#endif
                        break;
                    }
                }
                /*deque is stable*/
                else if (status == STABLE) {
                    /**
                     * Put left and right end of deque into SMR to prevent
                     * deletion from other threads.
                     */
                    assert(right != left);
                    mm->employ(hp, 0, left);
                    if (left != anchor.getLeft())
                        continue;
                    
                    mm->employ(hp, 1, right);
                    if (right != anchor.getRight())
                        continue;
                    
                    if (!(anchor == old_anchor))
                        continue;

                    prev = right->left.load(memory_order_relaxed);
                    mm->employ(hp, 2, prev);
                    if (right != anchor.getRight())
                        continue;
                    assert(NULL != prev);
#ifdef DEBUG_X86
                    if(prev==NULL) {
                        dumpDeque(old_anchor.value);
                        assert(prev!=NULL);
                    }
#endif
                    /*create a new anchor <l, prev,s>*/
                    new_anchor.setLeft(left);
                    new_anchor.setRight(prev);
                    new_anchor.setStatus(status);
#ifdef PPC
                    AnchorValue<T>* oldan = old_anchor.pvalue.load(memory_order_relaxed);
                    AnchorType<T>::mm_av->employ(hp_av, 0, oldan);
                    if (oldan != old_anchor.pvalue.load(memory_order_relaxed))
                        continue;
#endif
                    /*
                     * Replace the anchor with new value.
                     */
                    if (anchor.compare_swap(&old_anchor, &new_anchor)) {
                        ret = right->data;
                        mm->retire(hp, 0);
                        mm->retire(hp, 1);
                        mm->delNode(hp, right);
#ifdef DEBUG_DEQUE
                        char log[50];
                        sprintf(log, "%d:popRNode: %x\n", (int)pthread_self(), (unsigned int)right);
                        logPop(log);
                        ct->popRNum++;
#endif
#ifdef PPC
                        AnchorType<T>::mm_av->retire(hp_av, 0);
                        AnchorType<T>::mm_av->delNode(hp_av, oldan);
#endif
                        break;
                    }
                }
                /*deque is unstable*/
                else {
                    stabilize(old_anchor);
                }
            }
            return true;
        }

#ifdef DEBUG_X86
        void dumpDeque(AnchorValue<T> an) {
            //print_backtrace();
            char buffer[4096];
            int pos = sprintf(buffer, "Local Anchor: left: %lx right: %lx status: %lx\n",
                    an.fields.left, an.fields.right, an.fields.status);
            pos = sprintf(buffer+pos, "Shared Anchor: left: %lx right: %lx status: %lx\n",
                    an.fields.left, an.fields.right, an.fields.status);
            cout <<buffer;
            pos =0;
            if(an.fields.left!=0) {
                pos += sprintf(buffer+pos, "From Left:\n");
                Node<T> * left = (Node<T> *) an.fields.left;
                Node<T> * right = an.getRight();

                Node<T> * current = left;
                int i =0;
                while(current != right) {
                    pos += sprintf(buffer+pos,
                            "Index: %d node %p left %p right %p\n",
                            i, current, current->left.load(), current->right.load());
                    i++;
                    current = current->right.load();
                }
            }
            cout << buffer;
        }
#endif
        /**
         * Remove one element from the left end of deque. If the deque
         * is empty, return false. else return true and assign the left
         * most element to the parameter.
         *
         * @param ret
         * 		the left most element of the deque
         * @return
         *      If the deque is not empty, return true. else return false.
         */
        virtual bool popLeft(T& ret) __attribute__((noinline)) {
            AnchorType<T> old_anchor, new_anchor;
            Node<T>* prev;
            HazardP * hp = mm->getHPRec();
#ifdef PPC
            typename SMR<AnchorValue<T>, 1>::HP_Rec* hp_av = AnchorType<T>::mm_av->getHPRec(); 
#endif
#ifdef DEBUG_DEQUE
            OPTimes* ct = pLocalCTimes->get();
            if ( NULL == ct)
            {
                ct = new OPTimes();
                pLocalCTimes->set(ct);
            } 
#endif 
            while (true) {
                anchor.copyTo(old_anchor);
                Node<T>* left = old_anchor.getLeft();
                Node<T>* right = old_anchor.getRight();
                unsigned long status = old_anchor.getStatus();
                if (right == 0) {
                    assert(left == 0);
                    assert(status == STABLE); 
                    return false;
                }

                if (left == right) {
                    mm->employ(hp, 1, left);
                    if (left != anchor.getLeft())
                        continue;
                    new_anchor.setStatus(status);
                    new_anchor.setLeft(0);
                    new_anchor.setRight(0);
#ifdef PPC
                    AnchorValue<T>* oldan = old_anchor.pvalue.load(memory_order_relaxed);
                    AnchorType<T>::mm_av->employ(hp_av, 0, oldan);
                    if (oldan != old_anchor.pvalue.load(memory_order_relaxed))
                        continue;
#endif
                    if (anchor.compare_swap(&old_anchor, &new_anchor)){
                        ret = left->data;
                        mm->retire(hp, 1);
                        mm->delNode(hp, left);
#ifdef DEBUG_DEQUE
                        char log[50];
                        sprintf(log, "%d:popLNode: %x\n", (int)pthread_self(), (unsigned int)left);
                        logPop(log);
                        ct->popLNum++;
#endif
#ifdef PPC
                        AnchorType<T>::mm_av->retire(hp_av, 0);
                        AnchorType<T>::mm_av->delNode(hp_av, oldan);
#endif
                        break;
                    }
                } else if (status == STABLE) {
                    /**
                     * for memory management only. Left and Right nodes
                     * are put into SMR so both of them won't be
                     * deleted by other threads.
                     */
                    mm->employ(hp, 0, left);
                    if (left != anchor.getLeft())
                        continue;

                    mm->employ(hp, 1, right);
                    if (right != anchor.getRight())
                        continue;
                    
                    assert(left != NULL);

                    if (!(anchor == old_anchor))
                        continue;

                    prev = left->right.load();
                    mm->employ(hp, 2, prev);
                    if (right != anchor.getRight())
                        continue;

                    assert(NULL != prev);
#ifdef DEBUG_X86
                    if(prev==NULL) {
                        dumpDeque(old_anchor.value);
                        assert(prev!=NULL);
                    }
#endif
                    new_anchor.setLeft(prev);
                    new_anchor.setRight(right);
                    assert(status == STABLE);
                    new_anchor.setStatus(status);
#ifdef PPC
                    AnchorValue<T>* oldan = old_anchor.pvalue.load(memory_order_relaxed); 
                    AnchorType<T>::mm_av->employ(hp_av, 0, oldan);
                    if (oldan != old_anchor.pvalue.load(memory_order_relaxed))
                        continue;
#endif
                    if (anchor.compare_swap(&old_anchor, &new_anchor)) {
                        ret = left->data;
                        mm->retire(hp, 0);
                        mm->retire(hp, 1);
                        mm->delNode(hp, left);
#ifdef DEBUG_DEQUE
                        char log[50];
                        sprintf(log, "%d:popLNode: %x\n", (int)pthread_self(), (unsigned int)left);
                        logPop(log);
                        ct->popLNum++;
#endif
#ifdef PPC
                        AnchorType<T>::mm_av->retire(hp_av, 0);
                        AnchorType<T>::mm_av->delNode(hp_av, oldan);
#endif
                        break;
                    }
                } else {
                    stabilize(old_anchor);
                }
            }
            return true;
        }

        /*
         * @brief Get the size of the deque at this point.
         *
         * Important Node: This function is not thread-safe
         *
         * @return the number of elements in the queue
         */
        int size() {
            AnchorType<T> old_anchor;
            anchor.copyTo(old_anchor);
            Node<T> * left = old_anchor.getLeft();
            Node<T> * right =old_anchor.getRight();
            
            if (left  == 0) {
                return 0;
            } else if (left == right) {
                return 1;
            } else {
                int _size = 2; 
                while (true) {
                    Node<T> * lNext = left->right.load(memory_order_relaxed);
                    if (lNext == right) {
                        break;
                    }
                    _size++;
                    left = lNext;
                }
                return _size;
            }
        }

        /**
         * @brief Check to see if queue is empty.
         *
         * @return true if empty, or false.
         */
        bool empty() {
            AnchorType<T> old_anchor;
            anchor.copyTo(old_anchor);
            return (old_anchor.getLeft() == 0 && old_anchor.getRight() ==0) ? true : false;
        }

        /**
         * @brief Get the right most element.
         *
         * @param ret
         * 			The right most element. It is valid if the return value is true.
         * @return
         * 			If the deque is not empty, return true. else return false.
         */
        bool peekRight(T& ret) {
            HazardP * hp = mm->getHPRec();

            while (true)
            {
                Node<T>* right = anchor.getRight();
                if (NULL == right)
                {
                    return false;
                } else {
                    mm->employ(hp, 0, right);
                    if (anchor.getRight() != right)
                        continue;
                    ret = right -> data;
                    mm->retire(hp, 0);
                    return true;
                }
            }
        }

        /**
         * @brief Get the left most element.
         *
         * @param ret
         * 			The left most element. It is valid if the return value is true.
         * @return
         * 			If the deque is not empty, return true. else return false.
         */
        bool peekLeft(T& ret) {
            HazardP * hp = mm->getHPRec();

            while (true)
            {
                Node<T>* left = anchor.getLeft();
                if (NULL == left)
                {
                    return false;
                } else {
                    mm->employ(hp, 0, left);
                    if (anchor.getLeft() != left)
                        continue;
                    ret = left -> data;
                    mm->retire(hp, 0);
                    return true;
                }
            }
        }
    private:
        /**
         * @brief change the status of deque from unstable to stable.
         *
         * @param a A thread local anchor
         */
        void stabilize(AnchorType<T>& a) {
            if (a.getStatus() == RPUSH) {
                stabilizeRight(a);
            } else {
                assert(a.getStatus() == LPUSH);
                stabilizeLeft(a);
            }
        }

        /**
         * @brief Change the status from RPUSH to STABLE
         *
         * @param a
         *      A thread local anchor
         */
        void stabilizeRight(AnchorType<T>& a) {
            Node<T> *prev, *prevnext;
            AnchorType<T> new_anchor;
            Node<T>* al = a.getLeft();
            Node<T>* ar = a.getRight();
            HazardP * hp = mm->getHPRec();
            /*
             * Prevent other threads from deletion to left and right nodes
             */
            mm->employ(hp, 0, al);
            if (al != anchor.getLeft())
                return ;

            mm->employ(hp, 1, ar);
            if (ar != anchor.getRight())
                return ;
            assert((ar != NULL) && (al != NULL));
#ifdef DEBUG_X86
            if(ar==NULL||al==NULL) {
                //this should never happen
                dumpDeque(a.value);
                assert(false);
            }
#endif
            if (!(anchor == a))
                return;
            
            prev = ar->left.load();
            
            assert(NULL != prev);

#ifdef DEBUG_X86
            if(prev==NULL) {
                // this should never happen
                dumpDeque(a.value);
                assert(prev!=NULL);
            }
#endif

            /*
             * prevent deletion from other threads
             */
            mm->employ(hp, 2, prev);
            if (ar != anchor.getRight())
                return ;

            if (!(anchor == a))
                return;

            prevnext = prev->right.load();
            
            if (prevnext != ar) {
                if (!(anchor == a))
                    return;

                if (! prev->right.compare_swap(prevnext, ar)) {
                    return;
                }
            }
            new_anchor.setRight(ar);
            new_anchor.setLeft(al);
            new_anchor.setStatus(STABLE);

            anchor.compare_swap(&a, &new_anchor);

            mm->retire(hp, 0);
            mm->retire(hp, 1);
            mm->retire(hp, 2);            
        }

        /**
         * Change the status from LPUSH to STABLE
         * @param
         *      A thread local anchor
         */
        void stabilizeLeft(AnchorType<T>& a) { 
            Node<T> *prev, *prevnext;
            AnchorType<T> new_anchor;
            Node<T>* al = a.getLeft();
            Node<T>* ar = a.getRight();
            HazardP * hp = mm->getHPRec();
            /*
             * prevent deletion from other threads
             */
            mm->employ(hp, 0, al);
            if (al != anchor.getLeft())
                return ;

            mm->employ(hp, 1, ar);
            if (ar != anchor.getRight())
                return ;
            assert((ar != NULL) && (al != NULL));
#ifdef DEBUG_X86
            if(ar==NULL||al==NULL) {
                //this should never happen
                dumpDeque(a.value);
                assert(false);
            }
#endif
            if (!(anchor == a))
                return;

            prev = al->right.load();
            assert(NULL != prev);

#ifdef DEBUG_X86
            if(prev==NULL) {
                // this should never happen
                dumpDeque(a.value);
                assert(prev!=NULL);
            }
#endif
            /*
             * prevent deletion from other threads
             */
            mm->employ(hp, 2, prev);
            if (al != anchor.getLeft())
                return ;

            if (!(anchor == a))
                return;

            prevnext = prev->left.load();
            if (prevnext != al) {
                if (!(anchor == a))
                    return;

                if (!prev->left.compare_swap(prevnext, al)) {
                    return;
                }
            }
            new_anchor.setRight(ar);
            new_anchor.setLeft(al);
            new_anchor.setStatus(STABLE);
            anchor.compare_swap(&a, &new_anchor);
            
            mm->retire(hp, 0);
            mm->retire(hp, 1);
            mm->retire(hp, 2);
        }
    };
}

#endif
