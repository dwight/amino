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
#ifndef AASORT_H
#define AASORT_H

//#define VERBOSE

namespace amino{ 
    void aa_sort(int * array, int * end);
    void incore_sort(int * array, int * length);
    void aa_merge(int * start, int * middle, int * end);
#ifdef DEBUG
    void printAll(int * array, int length);
    void printVector(int * array);
#endif
}

#endif
