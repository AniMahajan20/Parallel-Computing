Group members:
Shreyas Srikrishna 2017A7PS0162P
Aniruddha Mahajan 2017A7PS0145P
J Lakshmi Teja 2017A7PS0068P

To build AND run, use the command:
	make <no_of_proc> file_path="<filePath>" op="<Operation>" words="<wordList>" time=<time yes or no>
	eg: make np=4 file_path="test.txt" op="and" words="for the or and" time=y

Options:
op="and"; op="or"; time=n; time=y

If time=n, the results are printed.
If time=y, time taken is printed.

Rest of the arguments are self-explanatory.