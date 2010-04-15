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

#ifndef PARALLEL_SORT_H
#define PARALLEL_SORT_H

#include <algorithm>
#include <stdexcept>
#include <assert.h>

#include <amino/aasort.h>
#include <amino/executor.h>
#include <amino/future.h>
#include <amino/ftask.h>

#ifdef DEBUG
#include <sstream>
using namespace std;
#endif 

namespace amino{

    /**
     * @brief this class represents a sort task which can be executed by an
     * executor.
     *
     * @tparam _sort A function object which can be called with _sort(start, end)
     * @tparam rand_access_iter An iterator whch can be accessed in random order.
     */
    template<typename _sort, typename rand_access_iter>
        class SortTask:public Runnable{
            private:
                _sort fsorter;
                rand_access_iter fstart;
                rand_access_iter fend;
            public: 
                SortTask(rand_access_iter start, rand_access_iter end, _sort sorter){
                    fstart=start;
                    fend=end;
                    fsorter=sorter;
                }

                void * run(){
                    fsorter(fstart, fend);
                    return NULL;
                }
        };

    /**
     * @brief this class represents a merge task which can be executed by an
     * executor. The two merger area must be consecutive.
     *
     * @tparam _merger A function object which can be called with _merger(start, end)
     * @tparam rand_access_iter An iterator whch can be accessed in random order.
     */
    template<typename _merger, typename rand_access_iter>
        class MergerTask:public Runnable{
            private:
                _merger fmerger;
                rand_access_iter fstart;
                rand_access_iter fmiddle;
                rand_access_iter fend;
            public: 
                MergerTask(rand_access_iter start, rand_access_iter middle,
                        rand_access_iter end, _merger merger){
                    fstart=start;
                    fend=end;
                    fmiddle = middle;
                    fmerger=merger;
                }

                void * run(){
#ifdef DEBUG
                    stringstream ss;
                    ss<<"Merging: "<<fstart<<","<<fmiddle<<","<<fend<<endl;
                    cout<<ss.str();
#endif
                    fmerger(fstart, fmiddle, fend);
#ifdef DEBUG
                    cout<<"Merging Completed!\n";
#endif
                    return NULL;
                }
        };

    /**
     * @brief The most common version of parallel sort, which utilize sort and
     * merge inside STL in a parallel approach.
     */
    template<typename value_type, typename local_sort, typename merge_sort, 
        typename rand_access_iter, typename ExecutorType>
            void parallel_sort(rand_access_iter start, rand_access_iter end, 
                    local_sort _sort, merge_sort _merge, 
                    int threadNum, ExecutorType * executor){ 
                /* TODO: Considering division of data here. AASort requre
                   alignment to 16 */
                int length=end-start;
                int divNum = 64*threadNum;
                int step = length/divNum;
                if(step<32) {
                    step=32;
                    if(32>length)
                        step = length;
                    divNum = length/step;
                }
                int first_step= step+(length- step*divNum);

                typedef SortTask<local_sort, rand_access_iter> * pSortTask;
                typedef FutureTask *  pFutureTask;
                pSortTask * sortTasks= new pSortTask[divNum];
                FutureTask ** futures = new pFutureTask[divNum];
                sortTasks[0] = new SortTask<local_sort, rand_access_iter>(start,
                        start+first_step, _sort);
                futures[0] = new FutureTask(sortTasks[0]);
                executor->execute(futures[0]);

                vector<int> steps;
                steps.push_back(0);

                int pos=first_step; 
                steps.push_back(first_step);
                for(int i=1;i<divNum;i++){
#ifdef DEBUG
                    ostringstream output;
                    output<<"PSort POS: "<<pos<<endl;
                    cout<<output.str();
#endif
                    sortTasks[i] = new SortTask<local_sort, rand_access_iter>(start+pos,
                            start+pos+step, _sort);
                    futures[i] = new FutureTask(sortTasks[i]);
                    executor->execute(futures[i]);
                    pos+=step; 
                    steps.push_back(pos);
                }

                for(int i=0;i<divNum;i++){
                    futures[i]->get();
                    delete futures[i];
                    delete sortTasks[i];
                }

                delete [] futures;
                delete [] sortTasks;

                //Begin to merge
                typedef MergerTask<merge_sort, rand_access_iter> * pMergeTask;
                pMergeTask * mergeTasks = new pMergeTask[divNum/2];
                futures = new pFutureTask[divNum/2]; 
                while(steps.size()>2){
                    unsigned int pos = 0;
                    while(pos<steps.size()-2){
                        int start_i = steps[pos];
                        int middle_i = steps[pos+1];
                        int end_i = steps[pos+2];
                        int i=pos/2;
                        mergeTasks[i] = new MergerTask<merge_sort, rand_access_iter>(
                                start+start_i, start+middle_i, start+end_i,_merge);
#ifdef DEBUG
                        stringstream ss;
                        ss<<"Executing MergerTask "<<i<<endl;
                        cout<<ss.str();
#endif
                        futures[i] = new FutureTask(mergeTasks[i]);
                        executor->execute(futures[i]);

                        pos +=2;
                    } 

                    pos = 0;
                    while(pos<steps.size()-2){
                        int i=pos/2;
                        futures[i]->get();
                        delete futures[i];
                        delete mergeTasks[i];
                        pos += 2;
                    }

                    //remove middles from steps
                    pos = 0;
                    for(vector<int>::iterator i=steps.begin();i<steps.end();){
                        if(pos%2==0||(i+1)==steps.end())
                            i++;
                        else
                            i= steps.erase(i);
                        pos++;
                    }
                }

                delete [] futures;
                delete [] mergeTasks;

            } 

    template<typename ExecutorType, typename value_type>
        void parallel_sort(value_type * start, value_type * end, 
                int threadNum, ExecutorType * executor){
            void (* pSort)(value_type *, value_type *) = & std::sort<value_type*>;
            void (* pMerge)(value_type *, value_type *, value_type *) = & std::inplace_merge<value_type*>;
            parallel_sort<value_type>(start, end, pSort, pMerge, threadNum, executor);
        }

    /**
     * I keep this function here for future specilization of parallel_sort
     */
    //    template<typename ExecutorType>
    //        void parallel_sort(int * start, int * end, int threadNum, ExecutorType * executor){
    //            void (* pSort)(int *, int *) = & std::sort<int*>;
    //            void (* pMerge)(int *, int *, int *) = & std::inplace_merge<int*>;
    //            parallel_sort<int>(start, end, pSort, pMerge, threadNum, executor);
    //            //parallel_sort<int>(start, end, incore_sort, aa_merge, threadNum, executor);
    //        }
}

#endif
