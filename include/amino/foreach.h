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
 *  08-08-04  ganzhi     N/A        Initial version
 */
#ifndef AMINO_FOREACH_H
#define AMINO_FOREACH_H

#include <algorithm>

#include <amino/thread.h>
#include <amino/ftask.h>

namespace amino{
    /**
     * @brief Parallel version of STL foreach() function.
     *
     * This function will execute the foreach() in parallel with the help of an executor.
     *
     * @param exec The executor for executing multiple transform tasks in parallel.
     * @param begin The start position of input data
     * @param end The end position of input data (Exclusively). 
     * @param func  This function or function object will be call on each of the input element.
     */
    template<typename iteratorT, typename FuncT, typename Executor>
        void for_each(Executor& exec, iteratorT begin, iteratorT end, FuncT func){
            int procCount = getProcessNum();
            for_each(exec, procCount, begin, end, func);
        }

    /**
     * @brief This wrap the std::for_each into a runnable object.
     */
    template<typename iteratorT, typename FuncT>
        class CallStdForEach:public Runnable{
            private:
                iteratorT f_begin, f_end;
                FuncT* f_func;
            public:
                /**
                 * @brief A default contructor which enable creation of an array of
                 * instance of this class
                 */
                CallStdForEach(){}

                /**
                 * @brief setup parameter for calling std::for_each
                 */
                void setup(iteratorT begin, iteratorT end, FuncT& func){
                    f_begin = begin;
                    f_end = end;
                    f_func = &func;
                }

                void * run(){
                    std::for_each(f_begin, f_end, *f_func);
                    return NULL;
                }
        };

    /**
     * @brief Parallel version of STL foreach() function.
     *
     * This function will execute the foreach() in parallel with the help of an executor.
     *
     * @param exec The executor for executing multiple transform tasks in parallel.
     * @param divNum The suggested number for dividing multiple subtasks.
     * @param begin The start position of input data
     * @param end The end position of input data (Exclusively). 
     * @param func  This function or function object will be call on each of the input element.
     */
    template<typename iteratorT, typename FuncT, typename Executor>
        void for_each(Executor& exec, int divNum, iteratorT begin, iteratorT end, FuncT func){
            int len = end-begin;
            int step = len/divNum;
            if(step==0) {
                step=1;
                divNum = len;
            }

            typedef CallStdForEach<iteratorT, FuncT> Caller;
            Caller * pCallers = new Caller[divNum];
            FutureTask ** pFTasks = new FutureTask*[divNum];

            // Fork foreach callers and submit them to executor
            for(int i=0;i<divNum-1;i++){
                pCallers[i].setup(begin+i*step, begin+(i+1)*step, func);
                pFTasks[i] = new FutureTask(pCallers+i);
                exec.execute(pFTasks[i]);
            }

            // Deal with the last slice, which can have different size
            pCallers[divNum-1].setup(begin+(divNum-1)*step, end, func);
            pFTasks[divNum-1] = new FutureTask(pCallers+(divNum-1)); 
            exec.execute(pFTasks[divNum-1]);

            // Wait for result
            for(int i=0;i<divNum;i++){
                pFTasks[i]->get();
        		delete pFTasks[i];
            }
            delete[] pFTasks;
            delete[] pCallers;
        }
};

#endif
