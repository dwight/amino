#include <string>
#include <cctype>
#include <amino/cstdatomic>
#include <amino/thread.h>
#include <amino/tp_exec.h>
#include <amino/ftask.h>
#include <amino/transform.h>

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

// The total number of the vector
const static int NUM = 100;

int 
main(int argc, char* argv[])
{
    UnaryFunc<int> uf;
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

    // change each elemet to its twice 
    transform(exec, 2, it1, it2, it1, uf);
    exec.shutdown();
	exec.waitTermination();
    gettimeofday(&t2, NULL);

    int takes1 = (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec;
     
    cout<<"The execution time is: "<<takes1<<" microseconds!"<<endl;
 
    i = 0;
    for (; i<NUM; i++) {
        cout<<dataV[i]<<endl;
    }

    return 0;
}
