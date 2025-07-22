
# CSCE 3600 Shell and System Calls

## Project Overview
`newshell` is a custom shell program written in C for CSCE 3600. It supports both **interactive** and **batch** modes and implements core shell functionality including command execution, history, aliases, pipelining, redirection, path management, and signal handling.

---

## How to Compile
To compile the program, use:

```bash
gcc newshell.c -o newshell
```

---

## Interactive Mode
To run in Interactive Mode:

```bash
./newshell
```

This starts the program and presents the user with a prompt to enter commands.

---

## Batch Mode
To run in Batch Mode:

```bash
./newshell <batch_file>
```

This will read commands from the specified batch file and execute them.

---

## Alias Functionality

Alias functionality is only available in **Interactive Mode**. To view help, type:

```bash
man alias
```

### Alias Usage

- `alias <name>='<command>'` – Creates new alias  
- `alias -r <name>` – Removes a specified alias  
- `alias -c` – Removes all aliases  
- `alias` – Lists all aliases  
- `<name>` – Executes alias from alias list

You can also use an alias alongside arguments (e.g., `ll -a` if `ll` is an alias for `ls`).

---

## Path Command Usage

- `path` – Prints the current path list  
- `path + <dir>` – Appends a directory to the list  
- `path - <dir>` – Removes a directory from the list

---

## History Feature

- `myhistory` – Shows the last 20 commands  
- `myhistory -c` – Clears the history  
- `myhistory -e<number>` – Re-executes a command from history (e.g., `myhistory -e3`)

---

## Signal Handling

The shell ignores signals like `Ctrl+C` (`SIGINT`) and `Ctrl+Z` (`SIGTSTP`) to prevent itself from quitting or pausing. These signals are only passed to child processes running user commands. The shell manages foreground control using process groups to ensure the correct behavior.

---

## Pipelining

The shell supports pipelining with up to three commands:

```bash
ls | grep .c | wc
```

---

## Contributors

- Brianna Jackson  
- Lucero Torres  
- Anthony Ayala  
- Alexandre DeWolf

---

## Notes

- Redirection (`<`, `>`) is supported
- Commands separated by `;` are supported
- Works in both interactive and batch mode
