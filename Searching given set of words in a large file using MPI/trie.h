#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAXC 36

typedef struct TrieNode
{
    struct TrieNode *children[MAXC];
    int end_index;
    int length;
} TrieNode;

TrieNode *getNode();
void insert(TrieNode *root, const char *key, const int index);
TrieNode *next_node(struct TrieNode *node, const char ch);
int get_child_index(char ch);
