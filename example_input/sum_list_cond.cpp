/* sum_list_cond.cpp
Сумма списка с условием.
*/

#include "klee/klee.h"
#include <stdio.h>

struct List {
	int value;
 	List* node;
};


int sumFistMNode(List* node, unsigned m) {
	int res = 0;
	while (node != nullptr && m > 0) {
		int tmp = node->value;
		res = tmp > 10 ? tmp : 2 * tmp;
		node = node->node;
		--m;
	}
	return res;
}

int main() {
	List* node;	
	klee_make_symbolic(&node, sizeof(node), "node");
	int a = sumFistMNode(node, 4);
	return 0;
}

