# Smallsh
This project is from my OS1 course. It involves us writing our own *small* shell in C.

# Features
1) Print an interactive input prompt
2) Parse command line input into semantic tokens
3) Implement parameter expansion
  a) Shell special parameters $$, $?, and $!
  b) Generic parameters as ${parameter}
4) Implement two shell built-in commands: exit and cd
5) Execute non-built-in commands using the the appropriate EXEC(3) function.
6) Implement redirection operators ‘<’,  ‘>’ and '>>'
7) Implement the ‘&’ operator to run commands in the background
8) Implement custom behavior for SIGINT and SIGTSTP signals

# Try it out
I've included a makefile to compile the program. 

Examples:  
Testing multiple redirection operators
```
printf test\\n > testfile
printf Hello\ World!\\n > infile
printf asdfhjkl > garbagefile
cat < infile >> testfile > garbagefile > outfile
cat testfile infile garbagefile outfile
```

Testing cd
```
cd /tmp
pwd
cd
/tmp
pwd
```
