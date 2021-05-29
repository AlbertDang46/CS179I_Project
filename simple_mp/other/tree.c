#include <stdio.h>
#include <stdlib.h>

int total = 0;

struct node {
    char data[2]; // 8 bytes
    struct node* left; // = 8 bytes
    struct node* right; // = 8 bytes
} node;

struct node* newNode(char data) {
    // Allocate memory for new node
    struct node* node = (struct node*)malloc(sizeof(struct node));

    node->data[0] = data;
    node->data[1] = '\0';
    node->left = NULL;
    node->right = NULL;

    return (node);
}

void constructTree(struct node **root, int length) {

    if (length == 0) {
        return;
    }
    *root = newNode('a');
    total++;
    length--;

    int leftLength = length / 2;
    int rightLength = length / 2;

    if (length % 2 != 0) {
        leftLength = (length / 2) + 1;
    }

    constructTree(&(*root)->left, leftLength);
    constructTree(&(*root)->right, rightLength);
}

void traverseTree(struct node **root) { // pre-order traversal
    if(*root != NULL) {
        printf("value = %s\n", (*root)->data);
        traverseTree(&(*root)->left);
        traverseTree(&(*root)->right);
    }
}

void destroyTree(struct node **root) {
    if (*root != NULL) {
        destroyTree(&(*root)->left);
        destroyTree(&(*root)->right);
        free(*root);
    }
}

int main(){

    struct node* root;
    constructTree(&root, 1000000);
    traverseTree(&root);
    destroyTree(&root);

    printf("Total = %d\n", total);

    return 0;
}