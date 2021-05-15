#include <mpi.h>
#include "trie.h"

#define MAX_LINE_SIZE 5000

typedef struct LineInfo
{
    char line[MAX_LINE_SIZE];
    int lineno;
    int proc;
} LineInfo;

typedef struct WordInfo
{
    int index;
    int lineno;
    int pos;
    int proc;
} WordInfo;

char *read_file_chunk(MPI_File *file, int rank, int nproc, long long int *chunksize);
void handle_overlap(char *chunk, long long int *locstart, long long int *locend, int rank, int nproc);
void find_words(TrieNode *root, int num_words, bool is_and, char *chunk, long long int locstart, long long int locend, int rank, LineInfo **lines_p, int *lcount_p, WordInfo **words_p, int *wcount_p, int *num_lines);
bool is_valid_line(bool *isWord, int num_words, bool is_and);
void print_results(LineInfo *lines, int lcount, WordInfo *words, int wcount, char **argv, int num_lines, MPI_Datatype line_info, MPI_Datatype word_info, int rank, int nproc, char *time);
