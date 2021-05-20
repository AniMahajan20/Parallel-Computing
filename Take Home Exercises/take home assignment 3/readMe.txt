README FILE
--------------------------------
Contents:- openmp_classes.c,makefile, readMe.txt.

This code calculate the multiplication of 2 matrices using OpenMP. The number of thread used can be changed using a 'macro' called NO_OF_THREADS in the code.


--------------
How to run
---------------
Run make in terminal. Then use command:

./openmp_classes <dimension_of_the_matrices>


dimension here refers to the row/column count. 

---------------------------------------------
---------------------------------------------
OUTPUT
---------------------------------------------
	for <dimension_of_matrices> = 99
Time Taken in static scheduling: 0.044537
Time Taken in dynamic scheduling: 0.018150
Time Taken in guided scheduling: 0.021615
