#include <stdio.h>
#include <stdlib.h>


struct node {
	int number; // the job number
	int pid; // the process id of the a specific process
	struct node *next; // when another process is called you add to the end of the linked list
};
struct node *pHead = NULL;

void linkedForEach(struct node *pNode, void (*fn)()) {
	struct node *pPrevious = NULL;
	while(pNode != NULL) {
		fn(pNode, pPrevious);
		pPrevious = pNode;
		pNode = pNode->next;
	}
}

void linkedSlice(struct node *pNode, struct node *pPrevious, struct node **pHead) {
    if(pPrevious != NULL) {
        pPrevious->next = pNode->next;
    } else {
		*pHead = (pNode->next) ? pNode->next : NULL;
    }
	free(pNode);
}

void filterEvens(struct node *pNode, struct node *pPrevious) {
	if(pNode->pid % 2 != 0) {
		printf("Removing node %d\n", pNode->pid);
		linkedSlice(pNode, pPrevious, pHead);
	}
}

void printItem(struct node *pNode) {
	printf("NodeNum : %d\n", pNode->pid);
}

int main(int argc, char const *argv[]) {
	printf("Running\n");
	struct node n1 = {1, 1, NULL};
	struct node n2 = {2, 2, NULL};
	struct node n3 = {3, 3, NULL};
	// struct node n4 = {4, 4, NULL};

	pHead = &n1;
	n1.next = &n2;
	n2.next = &n3;
	// n3.next = &n4;

	printf("\n---- Entire List ----\n");
	linkedForEach(pHead, printItem);

	printf("\n---- Filtering List ----\n");
	linkedSlice(&n3, &n2, &pHead);

	printf("\n---- Final List ----\n");
	linkedForEach(pHead, printItem);

	return 0;
}
