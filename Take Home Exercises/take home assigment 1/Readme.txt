Readme
----------------------------------------------------------------------
1. How to execute
   (a) type 'make' in command prompt
   (b) run commmand 
	mpiexec -np <no_of_processes> ./result 

2. Processor used
	The code is run on a quad-core processor with x = 25.	

3. Speedup and Efficiency for n=1 to 10.


No. of processes(n)	Execution Time		Speedup 	Efficiency
-----------------------------------------------------------------------------
1			0.001142		1.000000	1.0000000
2			0.001056		1.081439	0.5407195
3			0.000661		1.727685	0.5758950
4			0.000650		1.756923	0.4392307
5			0.000669		1.707025	0.3414050
6			0.000742		1.539083	0.2565138
7			0.000787		1.451080	0.2072971
8			0.000829		1.377563	0.1721953
9			0.039976		0.028567	0.0031741
10			0.039996		0.028552	0.0028552
