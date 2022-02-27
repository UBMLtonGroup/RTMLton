c:
	gcc -o deadline deadlinesyscall.c -lpthread

sml:
	../../build/bin/mlton -debug true periodic.sml
	sudo ./periodic @RTMLton rtthreads true -- 
