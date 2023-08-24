## Lay out of the repository

`include/` directory contains header files

`glibc_util.c` and `glibc_util.h` contains the functions that checks for characters and tokens that are recognizable by the glibc library (i.e gcc will recognize these tokens when compiling)
- The point of these 2 files is to check for the difference between the tokens that exist in `C` but do not exist in `Golf`

`scanner.c` is where the program starts, here we initialize all the variables that are needed to read in tokens as well as some error checkings. 

### Notes

The difference between the scanner in this repository compared to that of the reference compiler provided by the professor is:
 - The reference compiler treats `^` as an unknown character, hence, when this character is encountered, the reference compiler will treat this as a warning instead of errors
 - The scanner in this repository treats the `^` as an error since it is the `XOR` bitwise operator. The reason for this is to keep the consistency in outputing error messages that match with the `AND` and `OR` bitwise operators. 

 Function documentations can be found in `.h` files for shared functions
 Static (non-shared) function documentations are found in the source files. For ease of readability, all static functions are declared at the top of `.c` files
