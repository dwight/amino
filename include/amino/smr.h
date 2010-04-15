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
 *  Change History:
 *
 *  yy-mm-dd  Developer  Defect     Description
 *  --------  ---------  ------     -----------
 *  08-04-28  mojiong    N/A        Initial implementation
 *  08-06-15  ganzhi     N/A        Using pthread to retire hazard pointer
 *  08-07-15  ganzhi     singleton  Make SMR to singleton pattern
 */

#ifndef AMINO_SAFE_MEMORY_RECLAIM
#define AMINO_SAFE_MEMORY_RECLAIM

#include <cassert>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string.h>
#include <stdexcept>

#include <amino/stdatomic.h>
#include <amino/util.h>
#include <amino/threadLocal.h>

/**
 * SMR way to prevent ABA for lock-free data structrue
 * based on the following paper:
 * Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects
 * MM Michael - IEEE TRANSACTIONS ON PARALLEL AND DISTRIBUTED SYSTEMS, 2004
 *
 * @author Mo Jiong Qiu, Zhi Gan (ganzhi@gmail.com)
 *
 */

#include <pthread.h>

#ifdef DEBUG
#include <sys/time.h>
#endif

namespace internal {

    using namespace amino;
    using namespace std;
	/**
	 * @brief This class can represents a node inside retired list of SMR, and
	 * a node inside the hazard pointer list.
	 *
	 * @tparam T Must be a pointer type which is protected by this SMR.
	 */
	template<typename T> class SMRListNode {
	public:
		SMRListNode* next;
		T data;
	};

	/**
	 * @brief This is the class to store hazard point.
	 *
	 * It will be linked to a list throuth the "next" field. Each thread will
	 * have only one HPRecType object and it's read-only to other threads.
	 * Whenever a thread wants to delete a node, it will put the node into the
	 * retire list (rlist). And when rlist contains more than RH elements,
	 * thread will scan the rlist and release any node which is not contained
	 * in HPRecType objects of all threads.
	 *
	 * @tparam T The type of node stored in SMR
	 * @tparam K The upper limitation of number of hazard pointers per thread.
	 */
 
#define MEM_ALIGNED 1  //The memory allocated aligned.
#define MEM_NOALIGN 0  //The memory allocated not aligned.
    
	template<typename T, int K> class HPRecType {
	public:
		typedef T NodeType;
		typedef SMRListNode<NodeType*> SMR_Node;

		volatile NodeType *hp[K];
		HPRecType *next;
		// when a thread exit, active should be set to "false"
		atomic<bool> active;

		/*
		 * This is the list for retired nodes, which should be deleted
		 * if no other threads are using it.
		 * this is per-thread private variable.
		 */
		SMR_Node *rlist;

		/*
		 * Number of elements inside rlist.
		 * this is per-thread private variable.
		 */
		int rcount;

		/*
		 * number of threads at most created before me.
		 * once set, it will not change
		 */
		int nrThreads;

        /*
         * The memory allocation type. If 0, use new operator, else 1, use memory aligned allocator.
         */
        const int allocType;
#ifdef FREELIST
#define MAX_FREE_NODES 32
		SMR_Node *frListSMR;
		NodeType * frListData[MAX_FREE_NODES+1];

        /* number of smr nodes in free list */
		int fr_smr_count;
        /* number of data nodes in free list */
		int fr_data_count;

        /**
         * @brief Allocate data node from free list. Allocate from global heap if free
         * list is empty
         * @param allocType is the type for allocating memory, 0 means by new operator, 1 by memory aligned allocator. 
         * @return A data node which can be safely reused
         * */
		NodeType * allocDataNode() {
			if(fr_data_count==0) {
                return allocType == MEM_NOALIGN ? new NodeType() : NULL;
			}
			fr_data_count--;

			frListData[fr_data_count]->~NodeType();

#ifdef DEBUG
            verifySMRList();
#endif

            //using placement new to reuse space
			return allocType == MEM_NOALIGN? new (frListData[fr_data_count]) NodeType() : frListData[fr_data_count];
		}

        /**
         * @brief Allocate smr node from free list. Allocate from global heap if free
         * list is empty
         *
         * @return A smr node which can be safely reused
         */
		SMR_Node * allocSMRNode() {
			if(fr_smr_count==0) {
				return new SMR_Node();
			}
            assert(frListSMR != NULL);
			fr_smr_count--;
			SMR_Node * tmp = frListSMR;
#ifdef DEBUG
            if(frListSMR->next == NULL && fr_smr_count>0){
                cout << "SMR is in weird status" << endl;
                cout << frListSMR <<endl;
                cout << fr_smr_count;
                assert(frListSMR->next != NULL);
            }
#endif
			frListSMR = frListSMR->next;

#ifdef DEBUG
            verifySMRList();
#endif

            return tmp;
		}

        /**
         * @brief Put a data node into freelist. Node will be deleted if freelist is full
         *
         * @param node deleting node
         *
         */
		void freeData(NodeType * node) {
			if(fr_data_count<MAX_FREE_NODES) {
				frListData[fr_data_count] = node;
				fr_data_count++;
			}
			else  
			    allocType == MEM_NOALIGN? delete node : free(node);
#ifdef DEBUG
            verifySMRList();
#endif
		}

#ifdef DEBUG
        void verifySMRList(){
            int count =0;
            SMR_Node * p1;
            p1 = frListSMR;
            while(p1!=NULL){
                count++;
                if(count > fr_smr_count){
                    cerr << "Count is larger than fr_smr_count now!"<< count <<" " << fr_smr_count << endl;
                    p1 = frListSMR;
                    for(int i=0;i<count;i++){
                        cerr << p1 << " ";
                        p1 = p1->next;
                    }
                    assert(count == fr_smr_count);
                }
                p1 = p1->next;
            }
            assert(count == fr_smr_count);
        }
#endif

        /**
         * @brief Put a node into freelist. Node will be deleted if freelist is full
         *
         * @param smrNode deleting smr node
         *
         */
		void freeSMRNode(SMR_Node * smrNode) {
			if(fr_smr_count<MAX_FREE_NODES) {
				fr_smr_count++;
				smrNode->next = frListSMR;
				frListSMR=smrNode;
#ifdef DEBUG
                if(frListSMR->next==NULL && fr_smr_count>1){
                    cout<<fr_smr_count<<endl;
                    cout<<smrNode<<endl;
                    cout<<frListSMR<<endl;
                    assert(frListSMR->next!=NULL || fr_smr_count<=1);
                }
#endif
			}
			else
			    delete smrNode;
#ifdef DEBUG
            verifySMRList();
#endif
		}
#endif

		HPRecType(int atype = 0) : allocType(atype){
			init();
		}

        void init(){
			memset(this, 0, sizeof(HPRecType));
        }

#ifdef FREELIST
        void cleanFreeList()
        {
			for(int i=0;i<fr_smr_count;i++) {
				SMR_Node * tmp = frListSMR->next;
                delete frListSMR;
				frListSMR = tmp;
			}

			for(int i=0;i<fr_data_count;i++) {
				if (allocType == 0)
                    delete frListData[i];
                else { 
                    free(frListData[i]);
                }
			}

            fr_smr_count = 0;
            fr_data_count = 0;
            frListSMR = NULL;
        }
#endif

        ~HPRecType() {
#ifdef FREELIST
            cleanFreeList();
#endif
			//delete retired nodes in rlist, and rlist itself
			SMRListNode<NodeType *> *tmpListNode;

			while (rlist != NULL) {
				tmpListNode = rlist;
				rlist = rlist->next;
                if (allocType == MEM_NOALIGN)
                    delete tmpListNode->data;
                else 
                    free(tmpListNode->data);				
				delete tmpListNode;
#ifdef DEBUG
				rcount--;
#endif
			}
#ifdef DEBUG
			if(rcount)
			{
				cout<<"fatal error: rcount or freeCount not zeor"<<endl;
				cout<<"rcount="<<rcount<<endl;
			}
#endif
		}
	};

	/**
	 * @brief This function is used to clean thread-local SMR data.
	 *
	 * It won't delete anything from rlist. Other threads should execute
	 * helpScan() to deal with this rlist.  This method doesn't delete hazard
	 * pointer. When another thread is created, it can reuse this Hazard
	 * Pointer. When a thread exits, this function will be called.
	 *
	 * @param thr_local pthread feeds this data pointer to this function. It's
	 * of type <code>void *</code> due to pthread is a C-API.
	 */
	template<typename TT, int K> void retireHPRec(void * thr_local) {
		HPRecType<TT, K> *my_hprec = (HPRecType<TT, K> *) thr_local;
		if (my_hprec != NULL) {
#ifdef FREELIST
            my_hprec->cleanFreeList();
#endif
			for (int i = 0; i < K; i++)
                my_hprec->hp[i] = NULL;
#ifdef PPC
            __asm__ __volatile__(
                    "sync;"
                    );
#endif
#ifdef DEBUG
            //cout << "Retiring HPRec: "<< my_hprec << endl << flush;
#endif
			my_hprec->active = false;
		}
	}

	template<typename T, int K> class SMR;

	/**
	 * @brief Factory method to return instance of SMR<T, K>.
	 *
	 * @tparam Type of node stored inside SMR.
	 * @tparam Number of hazard pointers for one thread.
	 *
	 * @return A SMR instance with the specified template parameter.
	 */
	template<typename T, int K>
        SMR<T, K> * getSMR(int atype = 0) {
            static SMR<T, K> global_smr(atype);
            return &global_smr;
        }

	/**
	 * @brief SMR way to prevent ABA for lock-free data structrue based on the
	 * following paper:
	 *
	 * Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects
	 *
	 * MM Michael - IEEE TRANSACTIONS ON PARALLEL AND DISTRIBUTED SYSTEMS, 2004
	 *
	 * This class is used to managed memory for lock free algorithm.
	 * For C program, we only need one SMR for the whole program. But
	 * For C++ program, we need type information so delete can work
	 * correctly. For each composition of template parameter, we will
	 * have one SMR instance.
	 *
	 * @author Mo Jiong Qiu, Zhi Gan (ganzhi@gmail.com)
	 *
	 * @tparam T The type of node stored in SMR.
	 * @tparam K Number of hazard pointers for one thread
	 */
	template<typename T, int K> class SMR {
	public:
		typedef T NodeType;
		typedef HPRecType<NodeType, K> HP_Rec;
		typedef SMRListNode<NodeType*> SMR_Node;
	private:
		ThreadLocal<HP_Rec *> * thread_local; //myhprec

		/*
		 *  header of hazard pointer list.
		 *  this list has one node which is HPRecType for every thread.
		 */
		atomic<HP_Rec *> head_hprec;

		//number of hp records in total
		atomic_uint hp_count;

		/**
		 * Minimal length of rlist
		 */
		static const int MINIMAL_RLIST_LEN = 16;

		/**
		 * This integer is used to control length of list of retired nodes.
		 * When rlist has more than RH elements, scan() methods will be called
		 * to scan and delete unnecessary elements. This value is at least
		 * MINIMAL_RLIST_LEN, or 2*hp_count.
		 */
		int RH;

        /*
         * The memory allocation type. If 0, use new operator, else 1, use memory aligned allocator.
         */
        int allocType;
		/**
		 * This method will scan the hazard pointers and retired pointer
		 * list of current thread. If there is no match, this method will
		 * delete pointers inside retired list.
		 */
		void scan(HP_Rec* head) {

			HP_Rec *hprec = head;
			volatile NodeType *hptr;
			unsigned long pc, nrcount;
			SMR_Node *tmpList = NULL;

			HP_Rec *my_hprec = thread_local->get();
			SMR_Node *rlist = my_hprec->rlist;
			unsigned int max_threads = hprec->nrThreads;

			//this array is supposed to be allocated from stack instead of heap.
			volatile NodeType* plist[max_threads * K];

			//stage 1. Scan HP list and insert non-null values into plist
			pc = 0;
			for (; hprec != NULL; hprec = hprec->next) {
				for (int j = 0; j < K; j++) {
					hptr = hprec->hp[j];
					if (hptr != NULL)
					plist[pc++] = hptr;
				}
			}

			assert(pc <= max_threads * K);
			// stage 2. sort plist, and search every retired node at plist
			sort(plist, plist + pc);
			volatile NodeType ** newlast = unique(plist, plist + pc);

			nrcount = 0;
			while (rlist != NULL) {
				SMR_Node *tmpListNode = rlist;
				rlist = rlist->next;
				if (binary_search(plist, newlast, tmpListNode->data)) {
					// if a retired node is still required by other threads,
					// we put it into a new list
					tmpListNode->next = tmpList;
					tmpList = tmpListNode;
					nrcount++;
				} else {
					// delete a retired node if no other thread is using it
#ifdef FREELIST
					my_hprec->freeData(tmpListNode->data);
					my_hprec->freeSMRNode(tmpListNode);
#else
					allocType == MEM_NOALIGN ? delete tmpListNode->data : free(tmpListNode->data);
					delete tmpListNode;
#endif
				}
			}
			//stage 4. Update the retired list to the new list
			my_hprec->rlist = tmpList;
			my_hprec->rcount = nrcount;
		}

		/**
		 * @brief merge an old list into a new list.
		 *
		 * The old list is <code>*oldList</code> and the new list if
		 * <code>*n</code>. Each of them with count
		 * <code>*countOld</code> and <code>*countNew</code> respectively.
		 * This method is only used by helpScan() method.
		 *
		 * @param oldList the old list
		 * @param countOld number of elements inside old list
		 * @param newList the new list
		 * @param countNew number of elements inside new list
		 */
		void mergeList(SMR_Node **oldList, int *countOld,
		               SMR_Node **newList, int *countNew) {
			SMR_Node *tmpListNode;
			if (oldList == NULL || *oldList == NULL)
			return;
			tmpListNode = *oldList;
			while (tmpListNode->next != NULL)
			tmpListNode = tmpListNode->next;
			tmpListNode->next = *newList;
			*newList = *oldList;
			*countNew += *countOld;
			*oldList = NULL;
			*countOld = 0;
		}

		/**
		 * @brief This method will scan inactive Hazard Pointer record and
		 * merge its retired list with retired list of current thread.
		 *
		 * @param head the node to start the scan process
		 */
		void helpScan(HP_Rec *head) {
			HP_Rec *hprec;
			HP_Rec *my_hprec = thread_local->get();

			for (hprec = head; hprec != NULL; hprec = hprec->next) {
				// If a Hazard Pointer is still active, we will skip it.
				if (hprec->active.load(memory_order_relaxed)
						== true) {
					continue;
				}
				// Let's try to mark this Hazard Pointer active temporarily, so
				// no two threads will modify one Hazard Pointer in parallel
				bool b = false; // need a l-value here
				if (hprec->active.compare_swap(b, true) == false) {
					continue;
				}
				//now it is safe to operate this "hprec". We will merge its rlist
				//with rlist of current thread now.
				mergeList(&hprec->rlist, &hprec->rcount, &my_hprec->rlist,
						&my_hprec->rcount);
				// Let's mark this Hazard Pointer active
				hprec->active.store(false, memory_order_release);
			}
		}

		/**
		 * @brief Allocate or find a hazard pointer for current thread.
		 *
		 * This method search the whole list of hazard pointers and try to
		 * locate an incative one. If failed, it will try to allocate a new
		 * hazard pointer.
		 *
		 * @return return a hazard pointer record for current thread
		 */
		HP_Rec * allocHPRec() {
			HP_Rec *hprec;
			HP_Rec *oldhead;

			//First try to reuse a retired HP record
			for (hprec = head_hprec.load(memory_order_relaxed); hprec != NULL; hprec
					= hprec->next) {
				if (hprec->active.load(memory_order_relaxed) == true) {
					continue;
				}
				bool b = false;
				/* if hprec is active, continue */
				if (!hprec->active.compare_swap(b, true)) {
					continue;
				}

#ifdef DEBUG
                //cout << "Reuse HPRec: "<< hprec << endl << flush;

                //assert(hprec->fr_smr_count == 0);
                //assert(hprec->fr_data_count == 0);
                //hprec->verifySMRList();
#endif
				//Succeeded in locking an inactive HP record
				thread_local->set(hprec);
                              //  hprec->init();
#ifdef DEBUG
			//	cout<<"reuse hp:"<<hprec<<"tid"<<pthread_self()<<endl;
#endif

				return hprec;
			}

			/* don't reorder this line with the other lines */
			hp_count++;
			if (hp_count.load(memory_order_relaxed) >= MINIMAL_RLIST_LEN / 2) {
				RH = hp_count.load(memory_order_relaxed) * 2;
			}

			/* allocate and push a new HP record. supposed to be initialised */
			hprec = new HP_Rec (allocType);
			hprec->active.store(true, memory_order_relaxed);

			// Insert this new hazard pointer into the head of list.
			do {//wait-free - max.num.of threads is finite
				oldhead = head_hprec.load(memory_order_relaxed);
				hprec->next = oldhead;
				hprec->nrThreads = hp_count.load(memory_order_relaxed);
			}while (head_hprec.compare_swap(oldhead, hprec) == false);

			/* Set this to thread_local variable */
			thread_local->set(hprec);
			return hprec;
		}

#ifdef DEBUG
	public:
		//to get some performance data
		atomic_uint newAllocs;
		atomic_uint total_requests;
		atomic_long t_newNode;
		atomic_long t_retireNode;
#endif
	private:
        SMR(int atype = 0) : allocType(atype) {
			thread_local
                = new ThreadLocal<HP_Rec*> (&internal::retireHPRec<NodeType, K>);
			hp_count = 0;
			RH = MINIMAL_RLIST_LEN;
			head_hprec.store(NULL, memory_order_relaxed);
#ifdef DEBUG
			newAllocs = 0;
			total_requests = 0;
			t_newNode = 0;
			t_retireNode = 0;
#endif
		}
	public:
		friend SMR<NodeType, K> * getSMR<NodeType, K> (int);

		~SMR() { //supposed to be called when its assotiated lock-free ds is to be freed
			HP_Rec *hprec =
				head_hprec.load(memory_order_relaxed);
			HP_Rec *tmp_hprec = NULL;
			while (hprec != NULL) {
				tmp_hprec = hprec;
#ifdef DEBUG
				if(tmp_hprec->active.load(memory_order_relaxed)) {
					HP_Rec *my_hprec = thread_local->get();
					if(tmp_hprec!=my_hprec)
					cout << "Warning some HP is active" << endl;
				}
#endif
				hprec = hprec->next;
				delete tmp_hprec;
			}
			delete thread_local;
		}

		/**
		 * @brief Lock-free program should call this function whenever a shared node
		 * should be deleted. This function delays the deletion of nodes until
		 * no other threads need it.
		 *
		 * @param node The node which should be deleted after all reference to it has expired
		 */
		void delNode(NodeType* node) {
			delNode(getHPRec(), node);
		}

		/**
		 * @brief Lock-free program should call this function whenever a shared node
		 * should be deleted. This function delays the deletion of nodes until
		 * no other threads need it.
		 *
		 * @param my_hprec A thread local hazard pointer record which can be got by calling
		 * getHPRec();
		 * @param node The node which should be deleted after all reference to it has expired
		 */
		void delNode(HP_Rec *my_hprec, NodeType* node) {
            //cout <<my_hprec << " " << pthread_self()<<endl<<flush;
			// compiler_barrier is here for stoping a bug with O3 option
			compiler_barrier();
#ifdef DEBUG
			struct timeval start, end;
			gettimeofday(&start, NULL);
#endif
#ifdef FREELIST
			SMR_Node *tmpListNode = my_hprec->allocSMRNode();
#else
			SMR_Node *tmpListNode = new SMR_Node();
#endif

			tmpListNode->data = node;
			tmpListNode->next = my_hprec->rlist;
			my_hprec->rlist = tmpListNode;
			my_hprec->rcount++;
			if (my_hprec->rcount >= RH) {
				HP_Rec *head =
				head_hprec.load(memory_order_relaxed);
				//helpScan(head); //we just scan the list started from this point instead of the whole one
				scan(head);
			}
#ifdef DEBUG
			gettimeofday(&end, NULL);
			t_retireNode += 1000000*(end.tv_sec-start.tv_sec) + (end.tv_usec-start.tv_usec);
#endif
			//prevent compiler reordering which cause O3 failure
			compiler_barrier();
		}

		/**
		 * @brief Get the thread local hazard pointer record
		 *
		 * @return hazard pointer record for current thread
		 */
		HP_Rec * getHPRec() {
			HP_Rec *my_hprec = thread_local->get();
			if (my_hprec == NULL) {
				my_hprec = allocHPRec();
			}
			return my_hprec;
		}

		/**
		 * @brief Assign NULL to one hazard pointer so that the original
		 * pointer is not marked as "in-use" any more.
		 *
		 * If a node was emplyed by one thread, and it's not needed
		 * after a while, this thread can call this method to explicitly
		 * remove this node from SMR from hazard pointer list. After this
		 * method is called, this thread won't prevent deletion of
		 * targeting pointer any more.
		 *
		 * @param my_hprec A thread local hazard pointer record which can be got by calling
		 * getHPRec();
		 *
		 * @param index Index of removing nodes. The removing pointer is
		 * inserted into SMR with this index by calling employ(int,
		 * NodeType *) method.
		 */
		void retire(HP_Rec * my_hprec, int index) {
			my_hprec->hp[index] = NULL;
		}

		void retire(HP_Rec * my_hprec, NodeType *p) {
            for(int i = 0; i < K; ++i) {
                if(my_hprec->hp[i] == p) {
                    my_hprec->hp[i] = NULL;
                }
            }
		}

		/**
		 * @brief Assign NULL to one hazard pointer so that the original
		 * pointer is not marked as "in-use" any more.
		 *
		 * If a node was emplyed by one thread, and it's not needed
		 * after a while, this thread can call this method to explicitly
		 * remove this node from SMR from hazard pointer list. After this
		 * method is called, this thread won't prevent deletion of
		 * targeting pointer any more.
		 *
		 * @param index Index of removing nodes. The removing pointer is
		 * inserted into SMR with this index by calling employ(int,
		 * NodeType *) method.
		 */
		void retire(int index) {
			retire(getHPRec(), index);
		}

		void retire(NodeType *p) {
			retire(getHPRec(), p);
		}

		/**
		 * @brief allocate a new node. This function will reuse nodes from freelist if the macro
		 * FREELIST is defined.
		 */
		NodeType * newNode() {
			return newNode(getHPRec());
		}

		/**
		 * @brief allocate a new node. This function will reuse nodes from freelist if the macro
		 * FREELIST is defined.
		 *
		 * @param my_hprec A thread local hazard pointer record which can be got by calling
		 * getHPRec();
		 */
		NodeType * newNode(HP_Rec * my_hprec) {
			return my_hprec->allocDataNode();
		}

		/**
		 * @brief Lock-free program should call this method to mark one
		 * node as in-use.
		 *
		 * @param my_hprec A thread local hazard pointer record which can be got by calling
		 * getHPRec();
		 *
		 * @param index index of inserting node. It should be less than template
		 *      paramete <code>K</code>.
		 * @param pointer pointer to the inserting node.
		 */
		void employ(int index, NodeType * pointer) {
			employ(getHPRec(), index, pointer);
		}

		/**
		 * @brief Lock-free program should call this method to mark one
		 * node as in-use.
		 *
		 * @param my_hprec A thread local hazard pointer record which can be got by calling
		 * getHPRec();
		 *
		 * @param index index of inserting node. It should be less than template
		 *      paramete <code>K</code>.
		 * @param pointer pointer to the inserting node.
		 */
		void employ(HP_Rec * my_hprec, int index, NodeType* pointer) {
			assert(index < K);
			my_hprec->hp[index] = pointer;

            SLFENCE;
        }
	};
}

#endif
