#ifndef __NODE_H__
#define __NODE_H__

#include <stdio.h>
#include <stdlib.h>

#define NO_NODE '~'

typedef struct Node {
	char key;
	struct Node *left, *right;
} Node;

Node* newNode(char key) {
	Node *temp = malloc(sizeof(Node));
	temp->key = key;
	temp->left = temp->right = NULL;

	return temp;
}

Node* buildTree(unsigned int size, char key) {
    if (size == 0) {
        return NULL;
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

char* deserialize(Node *root, char *list) {
	if (list[0] == NO_NODE) {
	    return list + 1;
    }

	root = newNode(list[0]);
	char *right_start = deserialize(root->left, list + 1);
	char *end_index = deserialize(root->right, right_start);

    return end_index;
}

void inorder(Node *root) {
	if (root) {
		inorder(root->left);
		printf("%c\n", root->key);
		inorder(root->right);
	}
}

#endif
