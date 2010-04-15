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
 *  08-08-26  Hui Rui    N/A        Initial implementation
 */

#ifndef AMINO_ACCUMULATE_H
#define AMINO_ACCUMULATE_H

#include <numeric>

#include <amino/thread.h>
#include <amino/ftask.h>

namespace amino {
    /**
     * @brief Do a summation operation on the range of [first, last).
     *
     * @param exec The executor for executing multiple transform tasks in parallel.
     * @param first The start position of input data
     * @param last The end position of input data (Exclusively). 
     */
    template<typename iteratorT, typename T, typename Executor>
        T accumulate(Executor exec, iteratorT first, iteratorT last) {
            int procCount = getProcessNum();
            return accumulate(exec, procCount, first, last);
        }

    /**
     * @brief Do a reduce operation on the range of [first, last)
     *
     * @param exec The executor for executing multiple transform tasks in parallel.
     * @param first The start position of input data
     * @param last The end position of input data (Exclusively). 
     * @param func This function or function object will be call recursively on
     * pairs of the input elements.
     */
    template<typename iteratorT, typename FuncT, typename T, typename Executor>
        T accumulate(Executor exec, iteratorT first, iteratorT last, FuncT func) {
            int procCount = getProcessNum();
            return accumulate(exec, procCount, first, last, func);
        }

    template<typename iteratorT, typename T>
        class CallStdAccumulate: public Runnable {
            private:
                iteratorT f_first, f_last;
                T result;
            public:
                CallStdAccumulate() {
                }
                void setup(iteratorT first, iteratorT last) {
                    f_first = first;
                    f_last = last;
                }

                T getResult() {
                    return result;
                }

                void* run() {
                    assert(f_first + 1 <= f_last);
                    result = std::accumulate(f_first + 1, f_last, *f_first);
                    return NULL;
                }
        };

    template<typename iteratorT, typename T, typename FuncT>
        class CallStdAccumulate_Func: public Runnable {
            private:
                iteratorT f_first, f_last;
                FuncT* f_func;
                T result;
            public:
                CallStdAccumulate_Func() {
                }

                void setup(iteratorT first, iteratorT last, FuncT& func) {
                    f_first = first;
                    f_last = last;
                    f_func = &func;
                }
                T getResult() {
                    return result;
                }

                void* run() {
                    result = std::accumulate(f_first + 1, f_last, *f_first, *f_func);
                    return NULL;
                }
        };

    /**
     * @brief Do a summation operation on the range of [first, last)
     *
     * @param exec The executor for executing multiple transform tasks in parallel.
     * @param first The start position of input data
     * @param last The end position of input data (Exclusively). 
     */
    template<typename iteratorT, typename T, typename Executor>
        T accumulate(Executor& exec, int threadNum, iteratorT first, iteratorT last) {
            const int MULTI_NUM = 3;

            if (last - first < MULTI_NUM * threadNum)
                return std::accumulate(first + 1, last, *first);

            int step = (last - first) / threadNum;

            typedef CallStdAccumulate<iteratorT, T> Caller;
            Caller* pCallers = new Caller[threadNum];
            FutureTask **pFTasks = new FutureTask*[threadNum];
            T* result = new T[threadNum];

            for (int i = 0; i < threadNum - 1; ++i) {
                pCallers[i].setup(first + i * step, first + (i + 1) * step);
                pFTasks[i] = new FutureTask(pCallers + i);
                exec.execute(pFTasks[i]);
            }

            pCallers[threadNum - 1].setup(first + (threadNum - 1) * step, last);
            pFTasks[threadNum - 1] = new FutureTask(pCallers + threadNum - 1);
            exec.execute(pFTasks[threadNum - 1]);

            for (int i = 0; i < threadNum; ++i) {
                pFTasks[i] -> get();
                result[i] = pCallers[i].getResult();
            }
            T ret = std::accumulate(result + 1, result + threadNum, *result);

            delete[] pCallers;
            for (int i = 0; i < threadNum; ++i) {
                delete pFTasks[i];
            }
            delete[] pFTasks;
            delete[] result;

            return ret;
        }

    /**
     * @brief Do a reduce operation on the range of [first, last)
     *
     * @param exec The executor for executing multiple transform tasks in parallel.
     * @param first The start position of input data
     * @param last The end position of input data (Exclusively). 
     * @param func This function or function object will be call recursively on
     * pairs of the input elements.
     */
    template<typename iteratorT, typename T, typename FuncT, typename Executor>
        T accumulate(Executor& exec, int threadNum, iteratorT first, iteratorT last,
                FuncT func) {
            const int MULTI_NUM = 3;

            if (last - first < MULTI_NUM * threadNum)
                return std::accumulate(first + 1, last, *first);

            int step = (last - first) / threadNum;

            typedef CallStdAccumulate_Func<iteratorT, T, FuncT> Caller;
            Caller* pCallers = new Caller[threadNum];
            FutureTask **pFTasks = new FutureTask*[threadNum];
            T* result = new T[threadNum];

            for (int i = 0; i < threadNum - 1; ++i) {
                pCallers[i].setup(first + i * step, first + (i + 1) * step, func);
                pFTasks[i] = new FutureTask(pCallers + i);
                exec.execute(pFTasks[i]);
            }

            pCallers[threadNum - 1].setup(first + (threadNum - 1) * step, last, func);
            pFTasks[threadNum - 1] = new FutureTask(pCallers + threadNum - 1);
            exec.execute(pFTasks[threadNum - 1]);

            for (int i = 0; i < threadNum; ++i) {
                pFTasks[i] -> get();
                result[i] = pCallers[i].getResult();
            }
            T ret = std::accumulate(result + 1, result + threadNum, *result, func);

            delete[] pCallers;
            for (int i = 0; i < threadNum; ++i) {
                delete pFTasks[i];
            }
            delete[] pFTasks;
            delete[] result;

            return ret;
        }
}
;

#endif
