#include "trie.h"

TrieNode *getNode()
{
    TrieNode *n = (TrieNode *)malloc(sizeof(TrieNode));
    for (int i = 0; i < MAXC; i++)
        n->children[i] = NULL;
    n->end_index = -1;
    n->length = 0;
    return n;
}

void insert(TrieNode *root, const char *key, const int index)
{
    TrieNode *temp = root;

    int i;
    for (i = 0; key[i] != '\0'; i++)
    {
        int ch = get_child_index(key[i]);
        if (!temp->children[ch])
            temp->children[ch] = getNode();

        temp = temp->children[ch];
    }
    temp->end_index = index;
    temp->length = i;
}

TrieNode *next_node(struct TrieNode *node, const char ch)
{
    int ind = get_child_index(ch);
    if (!node)
        return NULL;
    return node->children[ind];
}

int get_child_index(char ch)
{
    int ind;
    if (isalpha(ch))
        ind = tolower(ch) - 'a';
    else if (isdigit(ch))
        ind = ch - '0' + 26;

    return ind;
}