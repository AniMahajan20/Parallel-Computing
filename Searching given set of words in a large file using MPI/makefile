all: run
	@mpirun -np ${np} ./run ${file_path} ${op} ${time} ${words}
run: search_parallel.c utils.c trie.c
	@mpicc search_parallel.c utils.c trie.c -o run
clean:
	rm -rf run