// RUN: %clangxx -I../../../include -g -DMAX_ELEMENTS=4 -fno-exceptions -emit-llvm -c -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --max-forks=25 --write-no-tests --exit-on-error --optimize --disable-inlining --search=nurs:depth --use-cex-cache %t1.bc

#include "klee/klee.h"
#include "klee/Internal/ADT/ImmutableSet.h"

using namespace klee;

typedef ImmutableSet<unsigned> T;

bool iff(bool a, bool b) {
  return !!a == !!b;
}

template<typename InputIterator, typename T>
bool contains(InputIterator begin, InputIterator end, T item) {
  for (; begin!=end; ++begin)
    if (*begin == item)
      return true;
  return false;
}

bool equal(T &a, T &b) {
  T::iterator aIt=a.begin(), ae=a.end(), bIt=b.begin(), be=b.end();
  for (; aIt!=ae && bIt!=be; ++aIt, ++bIt)
    if (*aIt != *bIt)
      return false;
  if (aIt!=ae) return false;
  if (bIt!=be) return false;
  return true;
}

template<typename InputIterator, typename T>
void remove(InputIterator begin, InputIterator end, T item) {
  InputIterator out = begin;
  for (; begin!=end; ++begin) {
    if (*begin!=item) {
      if (out!=begin)
        *out = *begin;
      ++out;
    }
  }
}

void check_set(T &set, unsigned num, unsigned *values) {
  assert(set.size() == num);

  // must contain only values
  unsigned item = klee_range(0, 256, "range");
  assert(iff(contains(values, values+num, item),
             set.count(item)));

  // each value must be findable, must be its own lower bound, and
  // must be one behind its upper bound
  for (unsigned i=0; i<num; i++) {
    unsigned item = values[i];
    assert(set.count(item));
    T::iterator it = set.find(item);
    T::iterator lb = set.lower_bound(item);
    T::iterator ub = set.upper_bound(item);
    assert(it != set.end());                // must exit
    assert(*it == item);                    // must be itself
    assert(lb == it);                       // must be own lower bound
    assert(--ub == it);                     // must be one behind upper
  }

  // for items not in the set...
  unsigned item2 = klee_range(0, 256, "range");
  if (!set.count(item2)) {
    assert(set.find(item2) == set.end());

    T::iterator lb = set.lower_bound(item2);
    for (T::iterator it=set.begin(); it!=lb; ++it)      
      assert(*it < item2);
    for (T::iterator it=lb, ie=set.end(); it!=ie; ++it)
      assert(*it >= item2);

    T::iterator ub = set.upper_bound(item2);
    for (T::iterator it=set.begin(); it!=ub; ++it)
      assert(*it <= item2);
    for (T::iterator it=ub, ie=set.end(); it!=ie; ++it)
      assert(*it > item2);
  }
}

#ifndef MAX_ELEMENTS
#define MAX_ELEMENTS 4
#endif

void test() {
  unsigned num=0, values[MAX_ELEMENTS];
  T set;

  assert(MAX_ELEMENTS >= 0);
  for (unsigned i=0; i<klee_range(0,MAX_ELEMENTS+1, "range"); i++) {
    unsigned item = klee_range(0, 256, "range");
    if (contains(values, values+num, item))
      klee_silent_exit(0);

    set = set.insert(item);
    values[num++] = item;
  }
  
  check_set(set, num, values);

  unsigned item = klee_range(0, 256, "range");  
  if (contains(values, values+num, item)) { // in tree
    // insertion is invariant
    T set2 = set.insert(item);
    assert(equal(set2, set));

    // check remove
    T set3 = set.remove(item);
    assert(!equal(set3, set)); // mostly just for coverage
    assert(set3.size() + 1 == set.size());
    assert(!set3.count(item));
  } else { // not in tree
    // removal is invariant
    T set2 = set.remove(item);
    assert(equal(set2, set));

    // check insert
    T set3 = set.insert(item);
    assert(!equal(set3, set)); // mostly just for coverage
    assert(set3.size() == set.size() + 1);
    assert(set3.count(item));
  }
}

int main(int argc, char **argv) {
  test();
  assert(T::getAllocated() == 0);
  return 0;
}
