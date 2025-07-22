Interactive Mode start-up:
To run this program in Interactive mode, you need to compile it as normal using "gcc newshell.c -o newshell". Then you can run it using "./newshell". This starts the program in Interactive Mode and presents the user with a prompt screen to input commands.

Batch Mode start-up:
To run this program in Batch Mode you need to compile it as normal using "gcc newshell.c -o newshell". Then you run it using the following command "./newshell <batch_file>". This will start the program in Batch Mode and it will immediately begin reading in commands from the batch file.

Alias Functionality:
To access the Alias functionality, the program must be started in Interactive Mode. Once started, you will be presented with a prompt screen. You can access a manual for all the alias commands by typing "man alias" directly into the prompt.
The alias functionality allows the user to create shortcuts/aliases for commands. When using aliases, the user can also use an alias in tandem with regular commands/arguments.

Alias Usage:

alias <name>='<command>'            : creates new alias and adds it to alias list
alias -r <name>                     : removes a specified alias from alias list
alias -c                            : removes all aliases from alias list
alias                               : lists all aliases within alias list
<name>                              : executes alias command from alias list

Path Usage:
path                                : prints the current path list
path + <dir>                        : appends a directory 
path - <dir>                        : removes a directory
