#include <string>
#include <cctype>
#include <amino/cstdatomic>
#include <amino/thread.h>
#include <amino/tp_exec.h>
#include <amino/ftask.h>
#include <amino/accumulate.h>

#include <vector>
#include <iostream>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

using namespace amino;

template<typename ParaType>
class UnaryFunc {
public:
	ParaType operator()(ParaType element) {
        return 2 * element;
	}
};

const static int NUM = 100;

int 
main(int argc, char* argv[])
{
    //UnaryFunc<int> uf;
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

    // Test the function 1 
    int result = accumulate<vector<int>::iterator, int, ThreadPoolExecutor>(exec, 2, it1, it2);
    exec.shutdown();
	exec.waitTermination();
    gettimeofday(&t2, NULL);

    int takes1 = (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec;
     

    // Test the function 2
    
    cout<<"The execution time is: "<<takes1<<" microseconds!"<<endl;
 
    cout<<"Result: "<<result<<endl;

    return 0;
}
