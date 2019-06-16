// file foldexample.cc - in the public domain - by Basile Starynkevitch
// compile it with g++ -Wall -O2 -S -fverbose-asm foldexample.cc
// the look inside the generated foldexample.s
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>

extern "C" int sum_of_squared_ints_in_vector(const std::vector<int>&v);

int sum_of_squared_ints_in_vector(const std::vector<int>&v) {
  return std::accumulate(v.begin(),v.end(), 0,
			 [](int x, int y){return x*x + y*y;}
			 );
} // end of sum_of_ints_in_vector

extern "C" int ugly_sum_of_squared_ints_in_vector(const std::vector<int>&v);
int ugly_sum_of_squared_ints_in_vector(const std::vector<int>&v) {
  int res = 0;
  for (auto i: v) {
    res += i*i;
  }
  return res;
} // end of ugly_sum_of_squared_ints_in_vector
