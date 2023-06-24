# Go-Compiler

This project compiles the Go Language Fragment (GoLF), which is a subset of the Go programming language, to MIPS assembly and output to `stdout`. 

Commits from this project was done in a private repository and recently made public. Hence, this repository was imported from that private repository and therefore no commit history can be found

# Build

Run `make` in the root directory of this project, which will create a binary executable. The binary is called `golf`

This `golf` executable takes in 1 input, which is the path of an input file and will print MIPS assembly code to `stdout` (or use the direct stream output to a file to create an assembly file (i.e `./golf input_file > output_file.s`)).  

## Lay out of the repository

`include/` directory contains header files

`glibc_util.c` and `glibc_util.h` contains the functions that checks for characters and tokens that are recognizable by the glibc library (i.e gcc will recognize these tokens when compiling)
- The point of these 2 files is to check for the difference between the tokens that exist in `C` but do not exist in `Golf`

`scanner.c` is where the program starts, here we initialize all the variables that are needed to read in tokens as well as some error checkings. 

## Example
The following code in `GoLF`:  

```
func main() {
    prints("hello world")
}
```

will result in the following assembly code to be generated (please note that since packages have to be imported in `Go` for `Println` to work, `GoLF` has a small alternative to use a newly defined function called `prints()` to print a string instead, in order to simplify this project from having external dependencies):

```
        .data
        Ltrue = 1
        Lfalse = 0
input:
        .space 2
        .align 2
Strue:
        .asciiz"true"
Sfalse:
        .asciiz"false"
        .text
        .globl main
F_printb:
        move $t0,$a0
        beqz $t0,Sfalse_print
        la $a0,Strue
        addi $v0, $0, 4
        syscall
        jr $ra
Sfalse_print:
        la $a0,Sfalse
        addi $v0, $0, 4
        syscall
        jr $ra
F_printc:
        addi $v0, $0, 11
        syscall
        jr $ra
F_printi:
        addi $v0, $0, 1
        syscall
        jr $ra
F_prints:
        addi $v0, $0, 4
        syscall
        jr $ra
F_getchar:
        addi $sp,$sp,-4
        sw $ra,0($sp)
        li $v0,8
        la $a0,input
        li $a1,2
        syscall
        lb $v0,input
        bne $v0,$zero,ret
        li $v0,-1
ret:
        lw $ra,0($sp)
        addi $sp,$sp,4
        jr $ra
F_divmodchk:
        beqz $a1,E_div_by_z
        seq $s0,$a0,-2147483648
        beqz $s0,L0
        seq $s0,$a1,-1
        beqz $s0,L0
        li $a1,1
L0:
        move $v0,$a1
        jr $ra
E_div_by_z:
        la $s0,S1
        move $a0,$s0
        jal F_prints
        j F_halt_err
E_must_return:
        la $s0,S2
        move $a0,$s0
        jal F_prints
        j F_halt_err
F_len:
        li $s0,0
F_len_loop:
        lb $s1,0($a0)
        beqz $s1,F_len_loop_done
        addi $a0,$a0,1
        addi $s0,$s0,1
        j F_len_loop
F_len_loop_done:
        move $v0,$s0
        jr $ra
main:
        subu $sp,$sp,4
        sw $ra,0($sp)
        la $t0,S3
        move $a0,$t0
        jal F_prints
FR_main:
        lw $ra,0($sp)
        addu $sp,$sp,4
        jr $ra
        j F_halt
F_halt:
        addi $v0, $0, 10
        syscall
F_halt_err:
        li $a0,1
        addi $v0, $0, 17
        syscall
        .data
S0:
        .byte 0
S1:
        .asciiz"error: division by zero\n"
S2:
        .asciiz"error: function must return a value\n"
S3:
        .byte 104
        .byte 101
        .byte 108
        .byte 108
        .byte 111
        .byte 32
        .byte 119
        .byte 111
        .byte 114
        .byte 108
        .byte 100
        .byte 0
```
