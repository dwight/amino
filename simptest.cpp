// run bin/select_platform.sh
// g++ -march=i486  -pthread -I include simptest.cpp && ./a.out

#include <iostream>
using namespace std;
#include "include/amino/stack.h"

int main() { 
  cout << "hello" << endl;

  amino::LockFreeStack<int> s;
  s.push(3);
  s.push(4);
  int i;
  cout << s.pop(i) << endl;
  cout << i << endl;

  return 0;
}
