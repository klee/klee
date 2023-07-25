#define NULL 0
#define OK 0
#define ERR 1
typedef struct Tree {
    struct Tree *next;
    int value;
} TreeNode;
extern void BusinessFunc(void);
extern int CheckValid(int value);

// case1: 普通的先解引用后判空场景
void TestErr1(TreeNode *stru)
{
    stru->value = 0;
    if (stru == NULL) { // CHECK-DAG: KLEE: WARNING: 100.00% NullCheckAfterDerefException True Positive at trace 3
        return;
    }
}

void TestErr2(TreeNode *stru, int *flag)
{
    if (CheckValid(stru->value) != OK) {
        return;
    }
    if (flag == NULL || stru == NULL) { // CHECK-DAG: KLEE: WARNING: 100.00% NullCheckAfterDerefException True Positive at trace 1
        return;
    }
    BusinessFunc();
    return;
}

// case2: 逻辑运算中，先解引用后判空

int TestErr3(int *arr) 
{
    if (arr[0] == 0 || arr == NULL) { // CHECK-DAG: KLEE: WARNING: 100.00% NullCheckAfterDerefException True Positive at trace 2
        return ERR;
    }
    return OK;
}

// case3: for循环头，初始语句中解引用，循环条件语句中判空
void TestErr4(TreeNode *head)
{
    TreeNode *node = NULL;
    TreeNode *nextNode = NULL;
    if (head == NULL) {
        return;
    }
    for (node = head->next, nextNode =node->next; node != NULL; node = nextNode, nextNode = node->next) {
        BusinessFunc();
    }
    return;
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --external-calls=all --annotations=%annotations --mock-policy=all --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --check-out-of-memory --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
