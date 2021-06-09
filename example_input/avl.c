#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "klee/klee.h"

struct node {
	int payload;
	int height;
	struct node * kid[2];
} dummy = { 0, 0, {&dummy, &dummy} }, *nnil = &dummy;
// internally, nnil is the new nul
 
struct node *new_node(int value)
{
	struct node *n = calloc(1, sizeof *n);
	*n = (struct node) { value, 1, {nnil, nnil} };
	return n;
}
 
int max(int a, int b) { return a > b ? a : b; }
inline void set_height(struct node *n) {
	n->height = 1 + max(n->kid[0]->height, n->kid[1]->height);
}
 
inline int ballance(struct node *n) {
	return n->kid[0]->height - n->kid[1]->height;
}
 
// rotate a subtree according to dir; if new root is nil, old root is freed
struct node * rotate(struct node **rootp, int dir)
{
	struct node *old_r = *rootp, *new_r = old_r->kid[dir];
 
	if (nnil == (*rootp = new_r))
		free(old_r);
	else {
		old_r->kid[dir] = new_r->kid[!dir];
		set_height(old_r);
		new_r->kid[!dir] = old_r;
	}
	return new_r;
}
 
void adjust_balance(struct node **rootp)
{
	struct node *root = *rootp;
	int b = ballance(root)/2;
	if (b) {
		int dir = (1 - b)/2;
		if (ballance(root->kid[dir]) == -b)
			rotate(&root->kid[dir], !dir);
		root = rotate(rootp, dir);
	}
	if (root != nnil) set_height(root);
}
 
// find the node that contains value as payload; or returns 0
struct node *query(struct node *root, int value)
{
	return root == nnil
		? 0
		: root->payload == value
			? root
			: query(root->kid[value > root->payload], value);
}
 
void insert(struct node **rootp, int value)
{
	struct node *root = *rootp;
 
	if (root == nnil)
		*rootp = new_node(value);
	else if (value != root->payload) { // don't allow dup keys
		insert(&root->kid[value > root->payload], value);
		adjust_balance(rootp);
	}
}
 
void delete(struct node **rootp, int value)
{
	struct node *root = *rootp;
	if (root == nnil) return; // not found
 
	// if this is the node we want, rotate until off the tree
	if (root->payload == value)
		if (nnil == (root = rotate(rootp, ballance(root) < 0)))
			return;
 
	delete(&root->kid[value > root->payload], value);
	adjust_balance(rootp);
}
 
// aux display and verification routines, helpful but not essential
struct trunk {
	struct trunk *prev;
	char * str;
};
 
void show_trunks(struct trunk *p)
{
	if (!p) return;
	show_trunks(p->prev);
	printf("%s", p->str);
}
 
// this is very haphazzard
void show_tree(struct node *root, struct trunk *prev, int is_left)
{
	if (root == nnil) return;
 
	struct trunk this_disp = { prev, "    " };
	char *prev_str = this_disp.str;
	show_tree(root->kid[0], &this_disp, 1);
 
	if (!prev)
		this_disp.str = "---";
	else if (is_left) {
		this_disp.str = ".--";
		prev_str = "   |";
	} else {
		this_disp.str = "`--";
		prev->str = prev_str;
	}
 
	show_trunks(&this_disp);
	printf("%d\n", root->payload);
 
	if (prev) prev->str = prev_str;
	this_disp.str = "   |";
 
	show_tree(root->kid[1], &this_disp, 0);
	if (!prev) puts("");
}
 
int verify(struct node *p)
{
	if (p == nnil) return 1;
 
	int h0 = p->kid[0]->height, h1 = p->kid[1]->height;
	int b = h0 - h1;
 
	if (p->height != 1 + max(h0, h1) || b < -1 || b > 1) {
		printf("node %d bad, balance %d\n", p->payload, b);
		show_tree(p, 0, 0);
		abort();
	}
	return verify(p->kid[0]) && verify(p->kid[1]);
}
 
#define MAX_VAL 32
int main(void)
{
	int x;
	struct node *root;
	klee_make_symbolic(&root, sizeof(root), "root");
 
	srand(time(0));
 
	for (x = 0; x < 10 * MAX_VAL; x++) {
		// random insertion and deletion
		if (rand()&1)	insert(&root, rand()%MAX_VAL);
		else		delete(&root, rand()%MAX_VAL);
 
		verify(root);
	}
 
	puts("Tree is:");
	show_tree(root, 0, 0);
 
	puts("\nQuerying values:");
	for (x = 0; x < MAX_VAL; x++) {
		struct node *p = query(root, x);
		if (p)	printf("%2d found: %p %d\n", x, p, p->payload);
	}
 
	for (x = 0; x < MAX_VAL; x++) {
		delete(&root, x);
		verify(root);
	}
 
	puts("\nAfter deleting all values, tree is:");
	show_tree(root, 0, 0);
 
	return 0;
}
