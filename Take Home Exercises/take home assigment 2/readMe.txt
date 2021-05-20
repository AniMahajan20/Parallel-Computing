README
*************************

HOW TO RUN THE CODE:

Compile and run code by typing : make
 
Then use:
./barrier_pthreads <no_of_threads>

For example:
	step1: make
	step2: ./barrier_pthreads 16


NOTE: Number of threads should be a power of 2 (like 8,16,128,1024 etc), since implementation of log barrier 
      fails for no_of_threads that are not powers of 2.

*******************************************************************************************************************

RESULTS:

                    Average Execution time (in sec)
                    linear barrier          log barrier
no_of_threads
    8                  0.000738               0.000892
    16                 0.001320               0.001647
    128                0.008383               0.011490
    1024               0.043610               0.073453
