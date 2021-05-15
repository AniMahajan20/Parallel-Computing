#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdbool.h>
#include <string.h>

void usage_check(int argc);
void open_file(char *filename, MPI_File *file, int rank);
void operator_check(char *op_str, bool *is_and);
void create_datatypes(int max_line_size, MPI_Datatype *line_info, MPI_Datatype *word_info);