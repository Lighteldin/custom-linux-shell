all:
	gcc -Wall -Wextra -g myShell.c -o myShell
	
clean:
	rm -f myShell