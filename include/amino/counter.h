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
 *  08-08-14  ganzhi     N/A        Initial implementation
 */
#ifndef AMINO_COUNTER
#define AMINO_COUNTER

#include <amino/util.h>
#include <amino/cstdatomic>

namespace amino{
    template<typename Num_Type, int copier_num>
        class Counter{
            typedef atomic<Num_Type> Atomic_Num;
            private:
                Atomic_Num counter_array[copier_num];
            public:
                Counter(){
                    for(int i=0;i<copier_num;i++)
                        counter_array[i] = 0;
                }

                /**
                 * @brief  increment the count by '1'
                 *
                 * @param rand a random number which helps to choose a random
                 * slot to operate 
                 */
                void increment(int rand){
                    increment(rand, 1);
                }

                /**
                 * @brief increment the count by value.
                 *
                 * @param rand a random number which helps to choose a random
                 * slot to operate
                 * @param value operation arguments
                 */
                void increment(int rand, int value){
                    /** 
                     * Two shif operation to avoid common patter which can
                     * cause performance problem.
                     */
                    rand |= rand>>16;
                    rand |= rand>>8;

                    counter_array[rand%copier_num].fetch_add(value, memory_order_relaxed);
                }

                /**
                 * @brief  decrement the count by '1'
                 *
                 * @param rand a random number which helps to choose a random
                 * slot to operate 
                 */
                void decrement(int rand){
                    decrement(rand, 1);
                }

                /**
                 * @brief decrement the count by value.
                 *
                 * @param rand a random number which helps to choose a random
                 * slot to operate
                 * @param value operation arguments
                 */
                void decrement(int rand, int value){
                    rand |= rand>>16;
                    rand |= rand>>8;

                    counter_array[rand%copier_num].fetch_sub(value, memory_order_relaxed);
                }

                /**
                 * @brief This will return the total count. The result might be
                 * inaccurate if parallel execution occured. In such case, it's
                 * possible for this method to return value that never appears
                 * in any moment.
                 */
                Num_Type load(){
                    Num_Type sum;
                    sum=0;
                    for(int i=0;i<copier_num;i++){
                        sum +=
                            counter_array[i].load(memory_order_relaxed);
                    }

                    return sum;
                }

                /**
                 * @brief This method set the counter to certain value. The
                 * result is inaccurate if parallel execution occured.
                 *
                 * @param value The new value of counter
                 */
                void store(Num_Type value){
                    Num_Type tmp = load();
                    increment((int) value%copier_num, value - tmp);
                }
        };
}


#endif
