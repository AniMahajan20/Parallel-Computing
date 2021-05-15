#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define NUM_CHARS 256

char *in_buf,
    **final_buf1, **final_buf2;
unsigned int freqs[NUM_CHARS];
char *codes[NUM_CHARS];
pthread_mutex_t lock;

typedef struct treenode
{
    int freq;
    unsigned char ch;
    struct treenode *left;
    struct treenode *right;
} treenode;

treenode *root;

typedef struct _pq
{
    int heap_size;
    treenode *A[NUM_CHARS];
} PQ;

typedef struct thread_args
{
    int rank;
    int start;
    int end;
} thread_args;

void create_pq(PQ *p)
{
    p->heap_size = 0;
}

int parent(int i)
{
    return (i - 1) / 2;
}

int left(int i)
{
    return i * 2 + 1;
}

int right(int i)
{
    return i * 2 + 2;
}

void heapify(PQ *p, int i)
{
    int l, r, smallest;
    treenode *t;

    l = left(i);
    r = right(i);

    if (l < p->heap_size && p->A[l]->freq < p->A[i]->freq)
        smallest = l;
    else
        smallest = i;
    if (r < p->heap_size && p->A[r]->freq < p->A[smallest]->freq)
        smallest = r;

    if (smallest != i)
    {
        t = p->A[i];
        p->A[i] = p->A[smallest];
        p->A[smallest] = t;
        heapify(p, smallest);
    }
}

void insert_pq(PQ *p, treenode *r)
{
    int i;

    p->heap_size++;
    i = p->heap_size - 1;

    while ((i > 0) && (p->A[parent(i)]->freq > r->freq))
    {
        p->A[i] = p->A[parent(i)];
        i = parent(i);
    }
    p->A[i] = r;
}

treenode *extract_min_pq(PQ *p)
{
    treenode *r;

    if (p->heap_size == 0)
    {
        printf("heap underflow!\n");
        exit(1);
    }

    r = p->A[0];

    p->A[0] = p->A[p->heap_size - 1];

    p->heap_size--;

    heapify(p, 0);
    return r;
}

void *get_frequencies(void *args)
{
    int r, n;
    thread_args *temp = (thread_args *)args;
    int start = temp->start;
    int end = temp->end;

    for (n = start; n <= end; n++)
    {
        r = in_buf[n];
        pthread_mutex_lock(&lock);
        freqs[r]++;
        pthread_mutex_unlock(&lock);
    }
}

treenode *build_huffman()
{
    int i, n;
    treenode *x, *y, *z;
    PQ p;

    create_pq(&p);

    for (i = 0; i < NUM_CHARS; i++)
    {
        if (freqs[i] > 0)
        {
            x = malloc(sizeof(treenode));

            x->left = NULL;
            x->right = NULL;
            x->freq = freqs[i];
            x->ch = (char)i;
            insert_pq(&p, x);
        }
    }

    n = p.heap_size - 1;

    for (i = 0; i < n; i++)
    {

        z = malloc(sizeof(treenode));
        x = extract_min_pq(&p);
        y = extract_min_pq(&p);
        z->left = x;
        z->right = y;

        z->freq = x->freq + y->freq;

        insert_pq(&p, z);
    }

    return extract_min_pq(&p);
}

void get_codes(treenode *r,
               int level,
               char code_so_far[],
               char *codes[])
{
    if ((r->left == NULL) && (r->right == NULL))
    {

        code_so_far[level] = 0;

        codes[r->ch] = strdup(code_so_far);
    }
    else
    {

        code_so_far[level] = '0';
        get_codes(r->left, level + 1, code_so_far, codes);

        code_so_far[level] = '1';
        get_codes(r->right, level + 1, code_so_far, codes);
    }
}

void *encode_and_decode_file(void *args)
{

    thread_args *temp = (thread_args *)args;
    int start = temp->start;
    int end = temp->end;
    int rank = temp->rank;
    char *out_buf = (char *)malloc(sizeof(char) * (end - start + 2));
    char *in_buff = (char *)malloc(sizeof(char) * (end - start + 2));

    unsigned char ch;
    char *s;

    char current_byte = 0;
    int nbits = 0;
    int total_nbits = 0;
    int nbytes = 0;

    for (int i = start; i <= end; i++)
    {
        char ch = in_buf[i];

        for (s = codes[ch]; *s; s++)
        {
            char b = *s;

            current_byte <<= 1;

            if (b == '1')
                current_byte |= 1;

            nbits++;
            total_nbits++;
            if (nbits == 8)
            {
                out_buf[nbytes] = current_byte;
                nbytes++;
                nbits = 0;
                current_byte = 0;
            }
        }
    }

    while (nbits)
    {
        current_byte <<= 1;
        nbits++;
        if (nbits == 8)
        {
            out_buf[nbytes] = current_byte;
            nbytes++;
            nbits = 0;
            current_byte = 0;
        }
    }

    out_buf[nbytes] = '\0';

    treenode *node = root;
    int in_ptr = 0;
    for (int i = 0; i < total_nbits; i++)
    {
        unsigned int num = out_buf[i / 8] & (1u << (7 - (i % 8)));
        if (num == 0)
        {
            node = node->left;
        }
        else
        {
            node = node->right;
        }

        if (node->left == NULL && node->right == NULL)
        {
            in_buff[in_ptr] = node->ch;
            in_ptr++;
            node = root;
        }
    }

    in_buff[in_ptr] = '\0';

    final_buf1[rank] = out_buf;
    final_buf2[rank] = in_buff;
}

int main(int argc, char *argv[])
{
    char code[NUM_CHARS], fname1[100], fname2[100];
    FILE *out_file1, *out_file2;

    clock_t time_start = clock();

    memset(freqs, 0, sizeof(freqs));

    FILE *input_file = fopen(argv[1], "r");
    if (input_file == NULL)
    {
        printf("%s does not exist\n", argv[1]);
        exit(2);
    }
    int num_threads = atoi(argv[2]);

    fseek(input_file, 0, SEEK_END);
    long long int file_size = ftell(input_file) - 1;
    fseek(input_file, 0, SEEK_SET);

    pthread_t threads[num_threads];
    in_buf = (char *)malloc(file_size * sizeof(char) + 1);
    final_buf1 = (char **)malloc(num_threads * sizeof(char *));
    final_buf2 = (char **)malloc(num_threads * sizeof(char *));
    printf("Reading file of size %lld bytes\n", file_size);

    int num = fread(in_buf, 1, file_size, input_file);
    printf("%d bytes read\n", num);

    printf("Getting frequencies of characters\n");
    pthread_mutex_init(&lock, NULL);
    for (int i = 0; i < num_threads; i++)
    {
        thread_args *temp = (thread_args *)malloc(sizeof(thread_args));
        temp->start = i * (file_size / num_threads);
        temp->end = (i + 1) * (file_size / num_threads) - 1;
        if (i == num_threads - 1)
            temp->end = file_size - 1;
        pthread_create(&threads[i], NULL, get_frequencies, temp);
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&lock);

    float seq_time_start = clock();
    printf("Building huffman tree\n");
    root = build_huffman(freqs);

    printf("Getting character codes\n");
    get_codes(root, 0, code, codes);

    sprintf(fname1, "%s-encoded", argv[1]);
    sprintf(fname2, "%s-decoded", argv[1]);
    out_file1 = fopen(fname1, "w");
    out_file2 = fopen(fname2, "w");

    clock_t seq_time_end = clock();
    float seq_seconds = (float)(seq_time_end - seq_time_start) / CLOCKS_PER_SEC;
    printf("Sequential portion of code completed in %f seconds\n", seq_seconds);

    for (int i = 0; i < num_threads; i++)
    {
        thread_args *temp = (thread_args *)malloc(sizeof(thread_args));
        temp->start = i * (file_size / num_threads);
        temp->end = (i + 1) * (file_size / num_threads) - 1;
        temp->rank = i;
        if (i == num_threads - 1)
            temp->end = file_size - 1;
        pthread_create(&threads[i], NULL, encode_and_decode_file, temp);
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
        fprintf(out_file1, "%s", final_buf1[i]);
        fprintf(out_file2, "%s", final_buf2[i]);
    }

    fclose(input_file);
    fclose(out_file1);
    fclose(out_file2);

    clock_t time_end = clock();
    float seconds = (float)(time_end - time_start) / CLOCKS_PER_SEC;
    printf("Encoding and decoding of file completed in %f seconds\n", seconds);
    printf("Sequential code ratio: %lf\n", seq_seconds / seconds);
    exit(0);
}
