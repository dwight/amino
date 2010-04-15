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
 *  08-08-26  Zhi Gan    N/A        Add comments
 */

#ifndef AMINO_TRANSFORM_H
#define AMINO_TRANSFORM_H

#include <algorithm>

#include <amino/thread.h>
#include <amino/ftask.h>

namespace amino {
    /**
     * @brief Parallel version of STL transform() function.
     *
     * This function will execute the transform() in parallel with the help of an executor.
     *
     * @param exec The executor for executing multiple transform tasks in parallel.
     * @param first The start position of input data
     * @param last The end position of input data (Exclusively). 
     * @param result The start position of result data
     * @param op  This function or function object will be call on each of the input element.
     */
    template<typename InputIterator, typename OutputIterator,
        typename UnaryFunction, typename Executor>
            OutputIterator transform(Executor& exec, InputIterator first,
                    InputIterator last, OutputIterator result, UnaryFunction op) {
                int procCount = getProcessNum();
                return transform(exec, procCount, first, last, result, op);
            }

    /**
     * @brief Parallel version of STL transform() function for two input
     * sequences and one output sequence.
     *
     * This function will execute the transform() in parallel with the help of an executor.
     *
     * @param exec The executor for executing multiple transform tasks in parallel.
     * @param first1 The start position of the 1st input data
     * @param last1 The end position of input data (Exclusively). 
     * @param first2 The start position of the 2nd input data
     * @param result The start position of result data
     * @binary_op  This function or function object will be call on each part of the input element.
     */
    template<typename InputIterator1, typename InputIterator2,
        typename OutputIterator, typename BinaryFunction, typename Executor>
            OutputIterator transform(Executor& exec, InputIterator1 first1,
                    InputIterator1 last1, InputIterator2 first2, OutputIterator result,
                    BinaryFunction binary_op) {
                int procCount = getProcessNum();
                return transform(exec, procCount, first1, last1, first2, result, binary_op);
            }

    /**
     * @brief A wrap class for calling std::transform.
     */
    template<typename InputIterator, typename OutputIterator,
        typename UnaryFunction>
            class CallStdTransformUnary: public Runnable {
                private:
                    InputIterator f_first, f_last;
                    OutputIterator f_result;
                    UnaryFunction *f_op;

                    OutputIterator f_result_last;

                public:
                    CallStdTransformUnary() {
                    }

                    /**
                     * @brief Input parameters for calling std::transform
                     */
                    void setup(InputIterator first, InputIterator last, OutputIterator result,
                            UnaryFunction& op) {
                        f_first = first;
                        f_last = last;
                        f_result = result;
                        f_op = &op;
                    }

                    /**
                     * @brief Return the result of std::transform
                     */
                    OutputIterator getResultLast() {
                        return f_result_last;
                    }

                    void* run() {
                        f_result_last = std::transform(f_first, f_last, f_result, *f_op);
                        return NULL;
                    }
            };

    /**
     * @brief A wrap class for calling std::transform. This wrap class deal
     * with two input sequences.
     */
    template<typename InputIterator1, typename InputIterator2,
        typename OutputIterator, typename BinaryFunction>
            class CallStdTransformBinary: public Runnable {
                private:
                    InputIterator1 f_first1, f_last1;
                    InputIterator2 f_first2;
                    OutputIterator f_result;
                    BinaryFunction *f_op;

                    OutputIterator f_result_last;

                public:
                    CallStdTransformBinary() {
                    }

                    void setup(InputIterator1 first1, InputIterator1 last1,	InputIterator2 first2, OutputIterator result,BinaryFunction& binary_op) {
                        f_first1 = first1;
                        f_last1 = last1;
                        f_first2 = first2;
                        f_result = result;
                        f_op = &binary_op;
                    }

                    OutputIterator getResultLast() {
                        return f_result_last;
                    }

                    void* run() {
                        f_result_last = std::transform(f_first1, f_last1, f_first2, f_result,
                                *f_op);
                        return NULL;
                    }
            };

    /**
     * @brief Parallel version of STL transform() function for one input
     * sequence and one output sequence.
     *
     * This function will execute the transform() in parallel with the help of an executor.
     *
     * @param exec The executor for executing multiple transform tasks in parallel.
     * @param threadNum Number of threads in exec argument
     * @param first The start position of the input data
     * @param last The end position of input data (Exclusively). 
     * @param result The start position of result data
     * @param op  This function or function object will be call on each part of the input element.
     */
    template<typename InputIterator, typename OutputIterator,
        typename UnaryFunction, typename Executor>
            OutputIterator transform(Executor& exec, int threadNum, InputIterator first,
                    InputIterator last, OutputIterator result, UnaryFunction op) {
                const int MULTI_NUM = 3;

                if (last - first < MULTI_NUM * threadNum)
                    return std::transform(first, last, result, op);

                int step = (last - first) / threadNum;

                typedef CallStdTransformUnary<InputIterator, OutputIterator, UnaryFunction>
                    Caller;
                Caller* pCallers = new Caller[threadNum];
                FutureTask **pFTasks = new FutureTask*[threadNum];

                for (int i = 0; i < threadNum - 1; ++i) {
                    pCallers[i].setup(first + i * step, first + (i + 1) * step, result + i
                            * step, op);
                    pFTasks[i] = new FutureTask(pCallers + i);
                    exec.execute(pFTasks[i]);
                }

                pCallers[threadNum - 1].setup(first + (threadNum - 1) * step, last, result
                        + (threadNum - 1) * step, op);
                pFTasks[threadNum - 1] = new FutureTask(pCallers + threadNum - 1);
                exec.execute(pFTasks[threadNum - 1]);

                for (int i = 0; i < threadNum; ++i) {
                    pFTasks[i] -> get();
                }

                OutputIterator ret = pCallers[threadNum - 1].getResultLast();

                delete[] pCallers;
                for (int i = 0; i < threadNum; ++i) {
                    delete pFTasks[i];
                }
                delete[] pFTasks;

                return ret;
            }

    /**
     * @brief Parallel version of STL transform() function for two input
     * sequences and one output sequence.
     *
     * This function will execute the transform() in parallel with the help of an executor.
     *
     * @param exec The executor for executing multiple transform tasks in parallel.
     * @param threadNum Number of threads in exec argument
     * @param first1 The start position of the 1st input data
     * @param last1 The end position of the 1st input data (Exclusively). 
     * @param first2 The start position of the 2nd input data
     * @param result The start position of result data
     * @binary_op  This function or function object will be call on each part of the input element.
     */
    template<typename InputIterator1, typename InputIterator2,
        typename OutputIterator, typename BinaryFunction, typename Executor>
            OutputIterator transform(Executor& exec, int threadNum, InputIterator1 first1,
                    InputIterator1 last1, InputIterator2 first2, OutputIterator result,
                    BinaryFunction op) {
                const int MULTI_NUM = 3;

                if (last1 - first1 < MULTI_NUM * threadNum)
                    return std::transform(first1, last1, first2, result, op);

                int step = (last1 - first1) / threadNum;

                typedef CallStdTransformBinary<InputIterator1, InputIterator2,
                        OutputIterator, BinaryFunction> Caller;
                Caller* pCallers = new Caller[threadNum];
                FutureTask **pFTasks = new FutureTask*[threadNum];

                for (int i = 0; i < threadNum - 1; ++i) {
                    pCallers[i].setup(first1+i*step, first1+(i+1)*step, first2+i*step, result+i*step, op);
                    pFTasks[i] = new FutureTask(pCallers + i);
                    exec.execute(pFTasks[i]);
                }

                pCallers[threadNum - 1].setup(first1 + (threadNum - 1) * step, last1,
                        first2 + (threadNum - 1) * step, result + (threadNum - 1) * step,
                        op);
                pFTasks[threadNum - 1] = new FutureTask(pCallers + threadNum - 1);
                exec.execute(pFTasks[threadNum - 1]);

                for (int i = 0; i < threadNum; ++i) {
                    pFTasks[i] -> get();
                }

                OutputIterator ret = pCallers[threadNum - 1].getResultLast();

                delete[] pCallers;
                for (int i = 0; i < threadNum; ++i) {
                    delete pFTasks[i];
                }
                delete[] pFTasks;

                return ret;
            }
}
;

#endif
