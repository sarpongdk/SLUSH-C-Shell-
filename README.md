# SLUSH

To begin interacting with the program, download slush and run the program

Alternatively, clone the repository and execute the command:
`make` in the directory to compile and run the program

### Description
slush - SLU shell

slush is a very simple command-line interpreter. It uses a different syntax than shells like bash, and has much less functionality. slush executes in a loop in which it displays a prompt, reads in a command line from standard input, and executes the command.

A built in command must appear on a line by itself. The only built in command is:

cd dir - change current directory to dir

### Usage
Program execution commands have the form:
```
slush ... prog_3 [args] ( prog_2 [args] ( prog_1 [args]
```
This command runs the programs prog_n, ... , prog_2, prog_1 (each of which may have zero or more arguments) as separate processes in a "pipeline". This means the standard output of each process is connected to the standard input of the next.

The syntax of slush is backwards from shells.

slush should catch ^C typed from the keyboard. If a command is running, this should interrupt the command. If the user is entering a line of input, slush should respond with a new prompt and a clean input line.

slush will exit when it reads an end-of-file on input.

