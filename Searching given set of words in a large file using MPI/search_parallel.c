#include "search_parallel.h"
#include "utils.h"

char *read_file_chunk(MPI_File *file, int rank, int nproc, long long int *chunksize)
{

	MPI_Offset globalstart, globalend, filesize;

	MPI_File_get_size(*file, &filesize);
	*chunksize = filesize / nproc;
	globalstart = (rank == 0 ? 0 : rank * (*chunksize) - 1);
	globalend = (rank == (nproc - 1) ? (filesize - 1) : ((rank + 1) * (*chunksize) - 1 + MAX_LINE_SIZE));
	*chunksize = globalend - globalstart + 1;
	char *chunk = malloc((*chunksize + 1) * sizeof(char));

	MPI_File_read_at_all(*file, globalstart, chunk, *chunksize, MPI_CHAR, MPI_STATUS_IGNORE);
	chunk[*chunksize] = '\0';

	//printf("Global %d: %lld %lld\n", rank, globalstart, globalend);
	return chunk;
}

void handle_overlap(char *chunk, long long int *locstart, long long int *locend, int rank, int nproc)
{

	if (rank != 0)
	{
		while (chunk[*locstart] != '\n')
			*locstart = *locstart + 1;
		*locstart = *locstart + 1;
	}

	if (rank != nproc - 1)
	{
		*locend -= MAX_LINE_SIZE;
		while (chunk[*locend] != '\n')
			*locend = *locend + 1;
	}

	//printf("Local %d: %lld %lld\n", rank, *locstart, *locend);
}

void find_words(TrieNode *root, int num_words, bool is_and, char *chunk, long long int locstart, long long int locend, int rank, LineInfo **lines_p, int *lcount_p, WordInfo **words_p, int *wcount_p, int *num_lines)
{

	TrieNode *temp = root;
	int lineno = 1, line_ptr = 0, line_wcount = 0, lcount = 0, wcount = 0;
	LineInfo ltemp;
	WordInfo wtemp;
	bool isWord[num_words];
	memset(isWord, false, sizeof(isWord));

	int lsize = (locend - locstart) / (MAX_LINE_SIZE);
	if (lsize <= 0)
		lsize = 1;
	int wsize = 3 * lsize;
	LineInfo *lines = (LineInfo *)malloc(lsize * sizeof(LineInfo));
	WordInfo *words = (WordInfo *)malloc(wsize * sizeof(WordInfo));

	for (long long int i = locstart; i <= locend; i++)
	{
		ltemp.line[line_ptr++] = chunk[i];

		if (isalnum(chunk[i]))
		{
			temp = next_node(temp, chunk[i]);
		}

		if (!isalnum(chunk[i]) || i == locend)
		{
			if (temp && temp != root && temp->end_index >= 0)
			{
				isWord[temp->end_index] = true;
				wtemp.index = temp->end_index;
				wtemp.lineno = lineno;
				wtemp.pos = line_ptr - temp->length - 1;
				if (i == locend)
					wtemp.pos++;
				wtemp.proc = rank;
				words[wcount++] = wtemp;
				if (wcount >= wsize)
				{
					wsize *= 2;
					words = (WordInfo *)realloc(words, wsize * sizeof(WordInfo));
				}
				line_wcount++;
			}
			temp = root;
		}

		if (chunk[i] == '\n' || i == locend)
		{

			bool is_valid = is_valid_line(isWord, num_words, is_and);
			if (!is_valid)
			{
				wcount -= line_wcount;
			}
			else
			{
				if (i != locend)
					line_ptr--;
				ltemp.line[line_ptr] = '\0';
				ltemp.lineno = lineno;
				ltemp.proc = rank;
				lines[lcount++] = ltemp;
				if (lcount >= lsize)
				{
					lsize *= 2;
					lines = (LineInfo *)realloc(lines, lsize * sizeof(LineInfo));
				}
			}
			memset(isWord, false, sizeof(isWord));
			line_wcount = line_ptr = 0;
			if (i != 0)
				lineno++;
		}
	}

	*lcount_p = lcount;
	*wcount_p = wcount;
	*num_lines = lineno - 1;
	*lines_p = lines;
	*words_p = words;
}

bool is_valid_line(bool *isWord, int num_words, bool is_and)
{

	bool is_valid = is_and;
	for (int i = 0; i < num_words; i++)
	{
		if (is_and)
			is_valid = is_valid && isWord[i];
		else
			is_valid = is_valid || isWord[i];
		if (is_valid != is_and)
			break;
	}

	return is_valid;
}

void print_results(LineInfo *lines, int lcount, WordInfo *words, int wcount, char **argv, int num_lines, MPI_Datatype line_info, MPI_Datatype word_info, int rank, int nproc, char *time)
{

	int lcounts[nproc];
	MPI_Gather(&lcount, 1, MPI_INT, lcounts, 1, MPI_INT, 0, MPI_COMM_WORLD);
	int ldisps[nproc];
	for (int i = 0; i < nproc; i++)
		ldisps[i] = (i > 0) ? (ldisps[i - 1] + lcounts[i - 1]) : 0;
	LineInfo *all_lines = NULL;
	if (rank == 0)
		all_lines = (LineInfo *)malloc((ldisps[nproc - 1] + lcounts[nproc - 1]) * sizeof(LineInfo));

	MPI_Gatherv(lines, lcount, line_info, all_lines, lcounts, ldisps, line_info, 0, MPI_COMM_WORLD);

	int wcounts[nproc];
	MPI_Gather(&wcount, 1, MPI_INT, wcounts, 1, MPI_INT, 0, MPI_COMM_WORLD);
	int wdisps[nproc];
	for (int i = 0; i < nproc; i++)
		wdisps[i] = (i > 0) ? (wdisps[i - 1] + wcounts[i - 1]) : 0;
	WordInfo *all_words = NULL;
	if (rank == 0)
		all_words = (WordInfo *)malloc((wdisps[nproc - 1] + wcounts[nproc - 1]) * sizeof(WordInfo));

	MPI_Gatherv(words, wcount, word_info, all_words, wcounts, wdisps, word_info, 0, MPI_COMM_WORLD);

	int line_disps[nproc];
	int num_lines_arr[nproc];
	MPI_Gather(&num_lines, 1, MPI_INT, num_lines_arr, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if (rank == 0)
	{

		for (int i = 0; i < nproc; i++)
		{
			line_disps[i] = (i > 0) ? (line_disps[i - 1] + num_lines_arr[i - 1]) : 0;
		}

		int total_words = wdisps[nproc - 1] + wcounts[nproc - 1];
		int total_lines = ldisps[nproc - 1] + lcounts[nproc - 1];
		int word_ptr = 0;
		printf("\n%d lines found!!\n--------------------\n\n", total_lines);
		for (int i = 0; i < total_lines; i++)
		{
			int proc = all_lines[i].proc;
			int lineno = all_lines[i].lineno;
			if (!strcmp(time, "n"))
				printf("Line No. %d: %s\n", lineno + line_disps[proc], all_lines[i].line);

			while (all_words[word_ptr].lineno == lineno && all_words[word_ptr].proc == proc)
			{
				WordInfo word = all_words[word_ptr];
				if (!strcmp(time, "n"))
					printf("%s: Position: %d\n", argv[word.index + 4], word.pos);
				word_ptr++;
				if (word_ptr == total_words)
					break;
			}
			if (!strcmp(time, "n"))
				printf("\n");
		}
	}
}

int main(int argc, char **argv)
{

	MPI_File file;
	int rank, nproc;
	double t1, t2, final_time;

	MPI_Init(&argc, &argv);

	MPI_Barrier(MPI_COMM_WORLD);
	t1 = MPI_Wtime();

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);

	MPI_Datatype line_info, word_info;
	create_datatypes(MAX_LINE_SIZE, &line_info, &word_info);

	open_file(argv[1], &file, rank);
	bool is_and;
	operator_check(argv[2], &is_and);

	TrieNode *root = getNode();
	for (int i = 4; i < argc; i++)
	{
		insert(root, argv[i], i - 4);
	}

	char *chunk;
	long long int chunksize;
	chunk = read_file_chunk(&file, rank, nproc, &chunksize);
	long long int locstart = 0, locend = chunksize - 1;
	handle_overlap(chunk, &locstart, &locend, rank, nproc);

	LineInfo *lines;
	WordInfo *words;
	int lcount, wcount, num_lines;
	find_words(root, argc - 4, is_and, chunk, locstart, locend, rank, &lines, &lcount, &words, &wcount, &num_lines);

	print_results(lines, lcount, words, wcount, argv, num_lines, line_info, word_info, rank, nproc, argv[3]);

	MPI_Barrier(MPI_COMM_WORLD);
	t2 = MPI_Wtime();
	double time = t2 - t1;
	MPI_Reduce(&time, &final_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	if (rank == 0 && !strcmp(argv[3], "y"))
	{
		printf("Time in seconds: %lf\n", final_time);
	}

	MPI_File_close(&file);

	MPI_Finalize();
	return 0;
}