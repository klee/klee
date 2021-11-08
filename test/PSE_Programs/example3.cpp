// RUN: %clangxx -I../../../include -g -std=c++11 -DMAX_ELEMENTS=4 -fno-exceptions -emit-llvm -c -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --max-forks=25 --write-no-tests --exit-on-error --optimize --disable-inlining --search=nurs:depth --use-cex-cache %t1.bc

#include <algorithm>
#include <klee/klee.h>
#include <stdio.h>
#include <string>
#include <vector>

template <class T>
std::string tostring(std::vector<T> &vec) {
  std::string str("");
  for (const auto &elems : vec) {
    str.append(std::to_string(elems));
    str.append(",");
  }
  str.pop_back();
  return str;
}

template <class T>
void make_pse_symbolic(void *addr, size_t bytes, const char *name, std::vector<T> dist) {
  klee_make_symbolic(addr, bytes, name);
  klee_assume(*(T *)addr >= *std::min_element(dist.begin(), dist.end()));
  klee_assume(*(T *)addr <= *std::max_element(dist.begin(), dist.end()));
}

/**
 * switch => door_switch
 */
bool montyhall(bool door_switch) {

  std::vector<int> car_door_dist = {0, 1, 2, 3};
  std::vector<int> choice_dist = {0, 1, 2, 3};

  int host_door = 0;
  int car_door, choice;

  make_pse_symbolic(&choice, sizeof(choice), "choice_pse_var_sym", choice_dist);
  make_pse_symbolic(&car_door, sizeof(car_door), "car_door_sym", car_door_dist);
  klee_make_symbolic(&host_door, sizeof(host_door), "host_door_sym");

  /**
   * Based on car door and choice, choose a host door.
   */
  if (car_door != 1 && choice != 1) {
    host_door = 1;
  } else if (car_door != 2 && choice != 2) {
    host_door = 2;
  } else {
    host_door = 3;
  }

  /**
   * Based door_switch and host_door, change choices.
   */
  if (door_switch) {
    klee_dump_kquery_state();
    if (host_door == 1) {
      if (choice == 2) {
        choice = 3;
      } else {
        choice = 2;
      }
    } else if (host_door == 2) {
      if (choice == 1) {
        choice = 3;
      } else {
        choice = 1;
      }
    } else {
      if (choice == 1) {
        choice = 2;
        klee_dump_symbolic_details(&choice, "choice_branch");
      } else {
        choice = 1;
      }
    }
  }

  if (choice == car_door) {
    return true;
  } else {
    return false;
  }

  return true;
}

int main() {
  int choice = 0;
  int door_switch = 0;
  std::vector<int> door_switch_dist = {0, 1};

  make_pse_symbolic(&door_switch, sizeof(door_switch), "door_switch_pse_var_sym", door_switch_dist);
  return montyhall(door_switch);
}