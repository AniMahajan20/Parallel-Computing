#include "utils.h"

void usage_check(int argc)
{

    if (argc < 4)
    {
        fprintf(stderr, "Usage: executable filepath AND/OR space_seperated_set_of_words\n");
        MPI_Finalize();
        exit(1);
    }
}

void open_file(char *filename, MPI_File *file, int rank)
{

    int err;
    err = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, file);
    if (err && rank == 0)
    {
        fprintf(stderr, "Couldn't open file %s\n", filename);
        MPI_Finalize();
        exit(2);
    }
}

void operator_check(char *op_str, bool *is_and)
{

    if (strcmp("OR", op_str) == 0 || strcmp("or", op_str) == 0)
    {
        *is_and = false;
    }
    else if (strcmp("AND", op_str) == 0 || strcmp("and", op_str) == 0)
    {
        *is_and = true;
    }
    else
    {
        fprintf(stderr, "%s is not a valid operator. The valid options are: OR, AND, or, and\n", op_str);
        MPI_Finalize();
        exit(3);
    }
}

void create_datatypes(int max_line_size, MPI_Datatype *line_info, MPI_Datatype *word_info)
{
    MPI_Datatype oldtypes1[2];
    int blockcounts1[2];
    MPI_Aint offsets1[2], extent;

    offsets1[0] = 0;
    oldtypes1[0] = MPI_CHAR;
    blockcounts1[0] = max_line_size;
    MPI_Type_extent(MPI_CHAR, &extent);
    offsets1[1] = max_line_size * extent;
    oldtypes1[1] = MPI_INT;
    blockcounts1[1] = 2;

    MPI_Type_struct(2, blockcounts1, offsets1, oldtypes1, line_info);
    MPI_Type_commit(line_info);

    MPI_Datatype oldtypes2[1];
    int blockcounts2[1];
    MPI_Aint offsets2[1];

    offsets2[0] = 0;
    oldtypes2[0] = MPI_INT;
    blockcounts2[0] = 4;

    MPI_Type_struct(1, blockcounts2, offsets2, oldtypes2, word_info);
    MPI_Type_commit(word_info);
}