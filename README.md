# Files searching
Searching a directory tree for files whose name matches some search term
## The goal
The goal of this assignment is to gain experience with threads and filesystem system calls. \
In this assignment, I create a program that searches a directory tree for files whose name matches
some search term. \
The program receives a directory D and a search term T, and finds every file in D’s directory tree whose name contains T.\
The program parallelizes its work using threads.
Specifically, individual directories are searched by different threads.

## The program flow:
The program create a FIFO queue to manage the threads, using a signals and condition variables.
## Execute program
Compile by: 
```
gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 -pthread pfind.c
```
run by:
```
./a.out arg1 arg2 arg3 
```
Such that:\
argv[1]: search root directory (search for files within this directory and its subdirectories).\
argv[2]: search term (search for file names that include the search term).\
argv[3]: number of searching threads to be used for the search (assume a valid integer greater than 0)

