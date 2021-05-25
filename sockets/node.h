#ifndef __NODE_H__
#define __NODE_H__

#include <stdio.h>
#define NO_NODE -1

struct Node {
	char key;
	struct Node *left, *right;
};

Node* newNode(char key) {
	Node *temp = new Node;
	temp->key = key;
	temp->left = temp->right = NULL;

	return temp;
}

Node* buildTree(unsigned int size, char key) {
    if (size == 0) {
        return 0;
    }

    unsigned int remainingSize = size - 1;
    unsigned int halfSize = remainingSize / 2;

    Node *root = newNode(key);
    root->left = buildTree(remainingSize - halfSize, key);
    root->right = buildTree(halfSize, key);

    return root;
}

char* serialize(Node *root, char *list) {
	if (root == NULL) {
        list[0] = NO_NODE;
		return list + 1;
	}

    list[0] = root->key;
	char *right_start = serialize(root->left, list + 1);
	char *end_index = serialize(root->right, right_start);

    return end_index;
}

char* deSerialize(Node *&root, char *list) {
	if (list[0] == NO_NODE) {
	    return list + 1;
    }

	root = newNode(list[0]);
	char *right_start = deSerialize(root->left, list + 1);
	char *end_index = deSerialize(root->right, right_start);

    return end_index;
}

void inorder(Node *root) {
	if (root) {
		inorder(root->left);
		printf("%d ", root->key);
		inorder(root->right);
	}
}

int main() {
	// Let us construct a tree shown in the above figure
	struct Node *root	 = newNode(20);
	root->left			 = newNode(8);
	root->right			 = newNode(22);
	root->left->left		 = newNode(4);
	root->left->right	 = newNode(12);
	root->left->right->left = newNode(10);
	root->left->right->right = newNode(14);

	// Let us open a file and serialize the tree into the file
	FILE *fp = fopen("tree.txt", "w");
	if (fp == NULL) {
		puts("Could not open file");
		return 0;
	}
	serialize(root, fp);
	fclose(fp);

	// Let us deserialize the storeed tree into root1
	Node *root1 = NULL;
	fp = fopen("tree.txt", "r");
	deSerialize(root1, fp);

	printf("Inorder Traversal of the tree constructed from file:\n");
	inorder(root1);

	return 0;
}

#endif
