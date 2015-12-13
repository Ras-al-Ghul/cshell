# cshell
Simple Unix shell in C.

### To execute
- `gcc shell.c -lpthread -o shell`
- `./shell`

### Features
- Supports a semicolon separated list of commands with their arguments.
- Can create background processes with `<command> &` and a message is displayed when finished.
- Input and Output redirection with `<`, `>` and `>>` have been implemented.
- Command redirection using `|` (any number) has been implemented.
- I/O redirection and Pipe redirection also work together.
- `cd`, `pwd`, `echo` and `quit` are implemented as built-in commands.
- `pinfo` is a command which when used without arguments prints process related info of the shell. `pinfo <pid>` prints process related info of the process with the given pid.
- `jobs` is a command which prints a list of currently executing processes in order of their creation, particularly background processes, along with their pid.
- `kjob <jobNumber> <signalNumber>` sends the indicated signal to the job with the given job number (got from `jobs`).
- `fg <jobNumber>` brings the background job with the given job number to the foreground.
- `overkill` is a command which kills all the background jobs.
- `CTRL-Z` changes the state of the currently running foreground job to `STOPPED` and pushes it to the background.
- Only `quit` exits the shell and all other keyboard interrupts are ignored.
- Error handling has been taken care of and appropriate messages are displayed.



