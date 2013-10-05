COMP3301 Assignment 3
=======

University of Queensland 2013 Semester 2 
Merrick Heley

This project uses user-mode linux to load the ext3301 file system.

New mount options:

	key=n
		Uses the last byte of the argument to form the encryption key

## Makefile ##
	make all
		calls "make module"
	
	make module
		make the ext3301 file system
		
	make ubdb
		make a test file system that can be mounted
		
	make clean
		clean the project directory
