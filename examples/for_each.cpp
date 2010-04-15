#include <iostream>

#include <amino/foreach.h>
#include <amino/thread.h>
#include <amino/ftask.h>
#include <amino/tp_exec.h>

#include <sys/time.h>
#include <unistd.h>

using namespace amino;

// global variable for sum result
int result = 0;

void 
sum (int n)
{
    result += n;
}

const static int NUM = 100;

int 
main(int argc, char * argv[]){
	vector<int> dataV;

    int i = 0;

    for ( ; i<NUM; i++) {
        dataV.push_back(i+1);        
    }
	
	ThreadPoolExecutor exec;
    
    vector<int>::iterator it1, it2;
    it1 = dataV.begin();
    it2 = dataV.end();

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    for_each(exec, 2, it1, it2, sum);
    exec.shutdown();
	exec.waitTermination();
    gettimeofday(&t2, NULL);

    // calculate the computing time
    int takes1 = (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec;
     
    cout<<"The execution time is: "<<takes1<<" microseconds!"<<endl;

    cout<<"result:"<<result<<endl;

    return 0;
} 
