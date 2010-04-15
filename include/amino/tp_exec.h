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
 *  08-07-28  ganzhi     N/A        Initial version
 */
#ifndef AMINO_UTIL_THREAD_EXECUTOR
#define AMINO_UTIL_THREAD_EXECUTOR

#include <pthread.h>
#include <string>
#include <stdio.h>
#include <memory>

#include <amino/thread.h>
#include <amino/exec_serv.h>
#include <amino/condition.h>
#include <amino/mutex.h>
#include <amino/lock.h>
#include <amino/ws_scheduler.h>
#include <amino/util.h>
#include <amino/future.h>
#include <amino/ftask.h>

#ifdef DEBUG_EXEC
#include <sstream>
#endif

namespace amino {
    using namespace std;

    class ThreadPoolExecutor;
    typedef ws_scheduler<Runnable> Scheduler;
    typedef Scheduler * pScheduler;

    /**
     * @brief An executor which pools a fix number of threads. Different tasks can reuse
     * threads among each execution.
     *
     * This class creates a fix number of threads at startup. Since then, these
     * threads are all blocked waiting for arrival of new tasks. Work stealing
     * scheduler is used internally to ensure efficient task scheduling.
     */
    class ThreadPoolExecutor:public ExecutorService{
        /**
         * @brief Worker thread contains an infinite loop which fetches and
         * executes tasks from scheduler.
         *
         * The infinite loop won't stop until a NULL task pointer is returned from
         * scheduler
         */
        class WorkerThread:public Thread{
            private:
                ThreadPoolExecutor * pExec;
                int threadId;
            public:
                WorkerThread(ThreadPoolExecutor * tp, int index){
                    pExec = tp;
                    threadId = index;
                }

                /**
                 * @brief fetch task from scheduler and execute it
                 * subclass.
                 */
                virtual void* run() {
                    while(true){
                        Runnable * task = pExec->scheduler->getTask(threadId);
                        //NULL means end of task queue
                        if(task==NULL){
#ifdef DEBUG_EXEC
                            ostringstream output;
                            output<<"Thread Exit: "<<pExec->active_count->load()<<endl;
                            cout<<output.str();
#endif
                            unique_lock<mutex> llock(pExec->f_mutex_finish);
                            int count = pExec->active_count->fetch_sub(1, memory_order_relaxed);
                            assert(count>0);
                            SC_FENCE;
                            if(count==1){
#ifdef DEBUG
                                cout<<"I'm notifying finish of running()\n";
#endif
                                pExec->notify_finish();
                            }
                            return NULL;
                        }

#ifdef DEBUG_EXEC
                        cout<<"A real task\n";
#endif
                        task->run();
                    }
                }
        };

        friend class WorkerThread;
        typedef WorkerThread * pThread;

        private:
        /**
         * scheduler has the duty to privde tasks to working threads
         */
        pScheduler scheduler;

        /* Number of working threads */
        int fThreadNum;

        /* Array of threads */
        WorkerThread ** threads;

        /* Count of working threads */
        atomic<int> * active_count;

        void init(int threadNum){
            fShutdown = false;

            fThreadNum = threadNum;

            scheduler = new Scheduler(fThreadNum);
            active_count = new atomic<int>();
            active_count->store(threadNum, memory_order_relaxed);

            threads = new pThread[threadNum];
            for(int i=0;i<threadNum;i++){
                threads[i] = new WorkerThread(this, i);
                threads[i]->start();
            }
        }

        public:
        ThreadPoolExecutor(){
            init(getProcessNum());
        }

        /**
         * @brief Create a thread pool executor which generate a fixed number of threads
         *
         * @param threadNum number of threads
         */
        ThreadPoolExecutor(int threadNum){
            init(threadNum);
        }

        /**
         * @brief destructor will delete all internal threads
         *
         * @exception std::logic_error when any thread is still working.
         */
        virtual ~ThreadPoolExecutor(){
            if(!finished()){
                if(!fShutdown)
                    shutdown();
                waitTermination();
                // There is no problem if you used waitTermination() and get this error.
                // Some exceptions might skipped your waitTermination()
                // throw logic_error("Some threads are still running");
            }

            for(int i=0;i<fThreadNum;i++){
                delete threads[i];
            }

            delete [] threads;
            delete active_count;

            delete scheduler;
        }

        /**
         * @brief Execute a task with a thread from internal thread pool.
         *
         * @exception std::logic_error when the executor is already shutdown.
         */
        void execute(Runnable* task){
            if(fShutdown)
                throw logic_error("Already shutdown\n!");
            //TODO what will happen if shutdown becomes true just at this moments?
            scheduler->addTask(task);
        }

        /**
         * @brief Mark the executor as shutdown state, which mean executor will
         * throw an exception when execute(Runnable*) or submit(Runnable *) are
         * called in future.
         */
        virtual void shutdown(){
            fShutdown = true;
            scheduler->shutdown();
        }

        virtual void halt(){
            fShutdown = true;
        }

        virtual void waitTermination(){
            for(int i=0;i<fThreadNum;i++){
                threads[i]->join();
            }
        }

        virtual bool waitTermination(int timeOut){
            bool res=ExecutorService::waitTermination(timeOut);
            if(res)
                for(int i=0;i<fThreadNum;i++){
                    threads[i]->join();
                }
#ifdef DEBUG
            if(!res)
                assert(!finished());

            if(res)
                assert(finished());

            stringstream ss;
            ss<<"Timed Wait Output: "<< res<<endl;
            cout<<ss.str();
#endif
            return res;
        }

        protected:
        virtual bool finished(){
#ifdef DEBUG
            ostringstream output;
            output<<"Finish_Active_Count: "<<active_count->load(memory_order_relaxed)<<" in "<<this<<endl;
            cout<<output.str()<<flush;
            assert(active_count->load(memory_order_relaxed)>=0);
#endif
            unique_lock<mutex> llock(f_mutex_finish);
            return active_count->load(memory_order_relaxed)==0;
        }
    };
}
#endif
